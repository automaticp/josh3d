#include "SSAO.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICore.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLProgram.hpp"
#include "GLTextures.hpp"
#include "Geometry.hpp"
#include "ImageData.hpp"
#include "MathExtras.hpp"
#include "Region.hpp"
#include "Tracy.hpp"
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


SSAO::SSAO(
    usize    kernel_size,
    float    deflection_rad,
    Extent2I noise_texture_resolution,
    usize    blur_kernel_limb_size)
{
    regenerate_kernel(kernel_size, deflection_rad);
    regenerate_noise_texture(noise_texture_resolution);
    resize_blur_kernel(blur_kernel_limb_size);
}

void SSAO::operator()(RenderEnginePrimaryInterface& engine)
{
    ZSCGPUN("SSAO");
    if (not enable_sampling) return;

    const auto* gbuffer = engine.belt().try_get<GBuffer>();

    if (not gbuffer) return;

    const Extent2I source_resolution = gbuffer->resolution();
    const Extent2I target_resolution = scaled_resolution(source_resolution, resolution_divisor);

    aobuffers._resize(target_resolution);

    const Extent2I noise_resolution = noise_texture_resolution();

    // Noise->Occlusion CoB such that `uv_noise = noise_scale * uv_occlusion`.
    const vec2 noise_scale = {
        float(target_resolution.width)  / float(noise_resolution.width),
        float(target_resolution.height) / float(noise_resolution.height),
    };

    glapi::set_viewport({ {}, target_resolution });

    // Sampling pass.
    {
        const RawProgram<> sp = _sp_sampling;

        sp.uniform("tex_depth",   0);
        sp.uniform("tex_normals", 1);
        sp.uniform("tex_noise",   2);
        sp.uniform("radius",      radius);
        sp.uniform("bias",        bias);
        sp.uniform("noise_scale", noise_scale);
        sp.uniform("noise_mode",  to_underlying(noise_mode));

        const BindGuard bsp = sp.use();

        const MultibindGuard bound_state = {
            gbuffer->depth_texture().  bind_to_texture_unit(0),
            _depth_sampler->           bind_to_texture_unit(0),
            gbuffer->normals_texture().bind_to_texture_unit(1),
            _normals_sampler->         bind_to_texture_unit(1),
            _noise_texture->           bind_to_texture_unit(2),
            _kernel.                   bind_to_ssbo_index(0),
            engine.                    bind_camera_ubo(),
        };
        glapi::unbind_sampler_from_unit(2); // Use internal noise texture sampler;

        _fbo->attach_texture_to_color_buffer(aobuffers._back().texture, 0);
        const BindGuard bfb = _fbo->bind_draw();
        glapi::clear_color_buffer(bfb, 0, RGBAF{ .r=0.f });

        engine.primitives().quad_mesh().draw(bsp, bfb);
        aobuffers._swap();
    }

    // Blur pass.
    if (blur_mode == BlurMode::Box)
    {
        const RawProgram<> sp = _sp_blur_box;
        sp.uniform("noisy_occlusion", 0);

        const BindGuard bsp = sp.use();

        const MultibindGuard bound_state = {
            aobuffers._front().texture->bind_to_texture_unit(0),
            _blur_sampler->             bind_to_texture_unit(0),
        };

        _fbo->attach_texture_to_color_buffer(aobuffers._back().texture, 0);
        const BindGuard bfb = _fbo->bind_draw();

        engine.primitives().quad_mesh().draw(bsp, bfb);
        aobuffers._swap();
    }

    if (blur_mode == BlurMode::Bilateral and
        // NOTE: We can skip the enitre blur pass
        // if the blur kernel size is 1 (limb is 0).
        blur_kernel_limb_size() > 0 and
        num_blur_passes > 0)
    {
        const RawProgram<> sp = _sp_blur_bilateral;
        sp.uniform("noisy_occlusion", 0);
        sp.uniform("depth",           1);
        sp.uniform("depth_limit",     depth_limit);

        const BindGuard bcam = engine.bind_camera_ubo();
        const BindGuard bsp  = sp.use();

        const MultibindGuard bound_state = {
            _blur_kernel.            bind_to_ssbo_index(0),
            _blur_sampler->          bind_to_texture_unit(0),
            gbuffer->depth_texture().bind_to_texture_unit(1),
            _depth_sampler->         bind_to_texture_unit(1),
        };

        for (const uindex _ : irange(num_blur_passes))
        {
            // Gaussian two-pass blur.
            for (const i32 blur_dim : { 0, 1 })
            {
                sp.uniform("blur_dim", blur_dim);

                aobuffers._front().texture->bind_to_texture_unit(0);
                _fbo->attach_texture_to_color_buffer(aobuffers._back().texture, 0);

                const BindGuard bfb = _fbo->bind_draw();
                engine.primitives().quad_mesh().draw(bsp, bfb);
                aobuffers._swap();
            }
        }
    }

    engine.belt().put_ref(aobuffers);
}

void SSAO::regenerate_kernel(usize n, float deflection_rad)
{
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

    _kernel.restage(ranges::views::generate_n(hemispherical_vec, n));
    _deflection_rad = deflection_rad;
}

void SSAO::regenerate_noise_texture(Extent2I resolution)
{
    std::normal_distribution<float> gaussian_dist;

    // We really don't care about magnitude since we just orthonormalize
    // this vector in the shader. So no need to normalize here.
    const auto gamma_fn = [&]{ return gaussian_dist(urbg_); };

    ImageData<float> imdata = { Extent2S(resolution), 3 }; // "image" of vec3's
    std::ranges::generate(imdata, gamma_fn);

    if (_noise_texture->get_resolution() != resolution)
    {
        _noise_texture = {};
        _noise_texture->allocate_storage(resolution, InternalFormat::RGB16F);
        // TODO: I wonder if setting filtering to linear and offseting differently
        // each repeat will produce better results?
        _noise_texture->set_sampler_min_mag_filters(MinFilter::Linear, MagFilter::Linear);
        _noise_texture->set_sampler_wrap_all(Wrap::Repeat);
    }

    _noise_texture->upload_image_region({ {}, resolution }, PixelDataFormat::RGB, PixelDataType::Float, imdata.data());
}

auto SSAO::blur_kernel_limb_size() const noexcept
    -> usize
{
    const usize n = _blur_kernel.num_staged();
    if (not n) return 0;
    return (n - 1) / 2;
}

void SSAO::resize_blur_kernel(usize limb_size)
{
    if (blur_kernel_limb_size() != limb_size)
    {
        if (limb_size == 0)
        {
            _blur_kernel.clear();
            _blur_kernel.stage_one(1.f);
        }
        else
        {
            const usize kernel_size = 2 * limb_size + 1;
            const float range       = float(limb_size);
            _blur_kernel.restage(
                generator_of_binned_gaussian_no_tails(-range, range, kernel_size));
        }
    }
}

} // namespace josh
