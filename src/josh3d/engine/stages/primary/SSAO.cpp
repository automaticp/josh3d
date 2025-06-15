#include "SSAO.hpp"
#include "GLAPIBinding.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLTextures.hpp"
#include "Geometry.hpp"
#include "ImageData.hpp"
#include "Region.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include <range/v3/view/generate_n.hpp>
#include <random>


namespace josh {

// TODO: Surely there must be a better place for this?
inline thread_local std::mt19937 urbg_{ std::random_device{}() };

namespace {

auto scaled_resolution(const Extent2I& resolution, float divisor) noexcept
    -> Extent2I
{
    return {
        i32(float(resolution.width)  / divisor),
        i32(float(resolution.height) / divisor),
    };
}

} // namespace


SSAO::SSAO(usize kernel_size, float deflection_rad, Extent2I noise_texture_resolution)
{
    regenerate_kernel(kernel_size, deflection_rad);
    regenerate_noise_texture(noise_texture_resolution);
}

void SSAO::operator()(RenderEnginePrimaryInterface& engine)
{
    if (not enable_sampling) return;

    const auto* gbuffer = engine.belt().try_get<GBuffer>();

    if (not gbuffer) return;

    const Extent2I source_resolution = gbuffer->resolution();
    const Extent2I target_resolution = scaled_resolution(source_resolution, resolution_divisor);

    aobuffers._resize(target_resolution);

    const Extent2I noise_resolution = noise_texture_resolution();

    // This is inverse of "noise scale" from learnopengl.
    // This is the size of a noise texture in uv coordinates of the screen
    // assuming the size of a pixel is the same for both.
    const vec2 noise_size = {
        float(noise_resolution.width)  / float(source_resolution.width),
        float(noise_resolution.height) / float(source_resolution.height),
    };

    glapi::set_viewport({ {}, target_resolution });

    // Sampling pass.
    {
        const RawProgram<> sp  = sp_sampling_;
        const MultibindGuard bound_state = {
            gbuffer->depth_texture().  bind_to_texture_unit(0),
            target_sampler_->          bind_to_texture_unit(0),
            gbuffer->normals_texture().bind_to_texture_unit(1),
            target_sampler_->          bind_to_texture_unit(1),
            noise_texture_->           bind_to_texture_unit(2),
            kernel_.                   bind_to_ssbo_index(0),
            engine.                    bind_camera_ubo(),
        };

        sp.uniform("tex_depth",   0);
        sp.uniform("tex_normals", 1);
        sp.uniform("tex_noise",   2);
        sp.uniform("radius",      radius);
        sp.uniform("bias",        bias);
        sp.uniform("noise_size",  noise_size);
        sp.uniform("noise_mode",  to_underlying(noise_mode));

        const BindGuard bsp = sp.use();
        const BindGuard bfb = aobuffers._fbo_noisy->bind_draw();

        glapi::clear_color_buffer(bfb, 0, RGBAF{ .r=0.f });
        engine.primitives().quad_mesh().draw(bsp, bfb);
    }

    // Blur pass.
    {
        const RawProgram<> sp = sp_blur_;
        sp.uniform("noisy_occlusion", 0);

        const MultibindGuard bound_state = {
            aobuffers.noisy_texture().bind_to_texture_unit(0),
            target_sampler_->         bind_to_texture_unit(0),
        };

        const BindGuard bsp = sp.use();
        const BindGuard bfb = aobuffers._fbo_blurred->bind_draw();

        engine.primitives().quad_mesh().draw(bsp, bfb);
    }

    // FIXME: This is not my responsibility, but everyone else suffers if I don't do this ;_;
    glapi::set_viewport({ {}, source_resolution });

    engine.belt().put_ref(aobuffers);
}

void SSAO::regenerate_kernel(usize n, float deflection_rad)
{
    if (n == kernel_.num_staged() and deflection_rad == deflection_rad_)
        return;

    std::normal_distribution<float>       gaussian_dist;
    std::uniform_real_distribution<float> uniform_dist;

    const float sin_deflection = glm::sin(deflection_rad);

    const auto hemispherical_vec = [&]()
        -> vec4
    {
        // What you find in learnopengl and the article it uses as a source
        // is not uniformly distributed over a hemisphere, but is instead
        // biased towards the covering-box vertices.
        //
        // 3d gaussian distribution is spherically-symmetric (we all watched
        // that video, right?), so we use that.
        vec3 vec;
        do
        {
            vec = {
                gaussian_dist(urbg_),
                gaussian_dist(urbg_),
                gaussian_dist(urbg_),
            };
            vec.z = glm::abs(vec.z);
            vec   = glm::normalize(vec);
        }
        while (dot(vec, Z) < sin_deflection);

        // Scaling our vector by random r in range [0, 1) will produce
        // the distribution of points in the final kernel with density
        // falling off as r^-2. (Volume element in spherical coordinates is ~ r^2 * dr)
        vec *= uniform_dist(urbg_);

        // TODO: Do we want r^-3 instead? If so, why?
        // What's the actual distribution of this that models physics as close as possible?
        //
        // I have no idea what they are doing at learnopengl at this point.
        // Wtf is an "accelerating interpolation function"?
        // It's just some number-mangling to me.

        return { vec, 0 };
    };

    kernel_.restage(ranges::views::generate_n(hemispherical_vec, n));
    deflection_rad_ = deflection_rad;
}

void SSAO::regenerate_noise_texture(Extent2I resolution)
{
    std::normal_distribution<float> gaussian_dist;

    // We really don't care about magnitude since we just orthonormalize
    // this vector in the shader. So no need to normalize here.
    const auto gamma_fn = [&]{ return gaussian_dist(urbg_); };

    ImageData<float> imdata = { Extent2S(resolution), 3 }; // "image" of vec3's
    std::ranges::generate(imdata, gamma_fn);

    if (noise_texture_->get_resolution() != resolution)
    {
        noise_texture_ = {};
        noise_texture_->allocate_storage(resolution, InternalFormat::RGB16F);
        // TODO: I wonder if setting filtering to linear and offseting differently
        // each repeat will produce better results?
        noise_texture_->set_sampler_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
        noise_texture_->set_sampler_wrap_all(Wrap::Repeat);
    }

    noise_texture_->upload_image_region({ {}, resolution }, PixelDataFormat::RGB, PixelDataType::Float, imdata.data());
}


} // namespace josh
