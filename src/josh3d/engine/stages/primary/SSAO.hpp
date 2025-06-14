#pragma once
#include "Attachments.hpp"
#include "EnumUtils.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLProgram.hpp"
#include "GLTextures.hpp"
#include "PixelData.hpp"
#include "PixelPackTraits.hpp"
#include "ShaderPool.hpp"
#include "UniformTraits.hpp"
#include "Pixels.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include "GLScalars.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <span>
#include <random>


namespace josh {


// SSAO output.
// TODO: This will no longer reference textures within lifetime after
// the SSAO stage will go out of scope. BAAAAAAD.
struct AmbientOcclusionBuffers {
    RawTexture2D<GLConst> noisy_texture;
    RawTexture2D<GLConst> blurred_texture;
};


} // namespace josh


namespace josh::stages::primary {


class SSAO {
public:
    // Switch to false if you want to skip the whole stage.
    bool  enable_occlusion_sampling{ true };
    float radius{ 0.2f };
    float bias{ 0.01f }; // Can't wait to do another reciever plane algo...
    float resolution_divisor{ 2.f }; // Used to scale the Occlusion buffer resolution compared to screen size.

    enum class NoiseMode : GLint {
        SampledFromTexture = 0,
        GeneratedInShader  = 1
    } noise_mode{ NoiseMode::GeneratedInShader };


    SSAO(const Extent2I& intial_resolution);

    void regenerate_kernels();
    size_t get_kernel_size() const noexcept { return sampling_kernel_->get_num_elements(); }
    void   set_kernel_size(size_t new_size) {
        resize_to_fit(sampling_kernel_, NumElems(new_size));
        regenerate_kernels();
    }

    float get_min_sample_angle_from_surface_rad() const noexcept { return min_sample_angle_rad_; }
    void  set_min_sample_angle_from_surface_rad(float angle_rad) {
        min_sample_angle_rad_ = angle_rad;
        regenerate_kernels();
    }

    void regenerate_noise_texture();
    Size2I get_noise_texture_size() const noexcept { return noise_size_; }
    void   set_noise_texture_size(const Size2I& new_size) {
        noise_size_ = new_size;
        regenerate_noise_texture();
    }

    auto view_output() const noexcept
        -> const AmbientOcclusionBuffers&
    {
        return *output_;
    }

    auto share_output_view() const noexcept
        -> SharedStorageView<AmbientOcclusionBuffers>
    {
        return output_.share_view();
    }

    void operator()(RenderEnginePrimaryInterface& engine);


private:
    ShaderToken sp_sampling_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ssao_sampling.frag")});

    ShaderToken sp_blur_ = shader_pool().get({
        .vert = VPath("src/shaders/postprocess.vert"),
        .frag = VPath("src/shaders/ssao_blur.frag")});

    using NoisyTarget   = RenderTarget<NoDepthAttachment, UniqueAttachment<Renderable::Texture2D>>;
    using BlurredTarget = RenderTarget<NoDepthAttachment, UniqueAttachment<Renderable::Texture2D>>;

    NoisyTarget   noisy_target_;
    BlurredTarget blurred_target_;


    SharedStorage<AmbientOcclusionBuffers> output_{
        noisy_target_  .color_attachment().texture(),
        blurred_target_.color_attachment().texture()
    };



    UniqueSampler target_sampler_{ []() {
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Linear, MagFilter::Linear);
        s->set_wrap_all(Wrap::ClampToEdge);
        return s;
    }() };


    Size2I scaled_resolution(const Size2I& resolution) const noexcept {
        return {
            GLsizei(float(resolution.width)  / resolution_divisor),
            GLsizei(float(resolution.height) / resolution_divisor),
        };
    }


    UniqueTexture2D noise_texture_;
    Size2I noise_size_{ 16, 16 };


    // It's still unclear to me why they call it "kernel".
    // I get that it *sounds like* convolution:
    //   "for each pixel get N samples and reduce that to one value"
    // But it is not convolution in a mathematical sense, no?


    // We use vec4 to avoid issues with alignment in std430,
    // even though we only need vec3 of data.
    UniqueBuffer<glm::vec4> sampling_kernel_ =
        allocate_buffer<glm::vec4>(NumElems{ 12 }, { .mode = StorageMode::StaticServer });

    float min_sample_angle_rad_{ glm::radians(5.f) };


    inline static thread_local std::mt19937 urbg_{};

};




inline SSAO::SSAO(const Extent2I& resolution)
    : noisy_target_  (scaled_resolution(resolution), { InternalFormat::R8 })
    , blurred_target_(scaled_resolution(resolution), { InternalFormat::R8 })
{
    regenerate_kernels();
    regenerate_noise_texture();
}




inline void SSAO::regenerate_kernels() {
    std::normal_distribution<float>       gaussian_dist;
    std::uniform_real_distribution<float> uniform_dist;

    auto hemispherical_vec = [&]() -> glm::vec4 {
        // What you find in learnopengl and the article it uses as a source
        // is not uniformly distributed over a hemisphere, but is instead
        // biased towards the covering-box vertices. Bad math is bad math, eeehhh...
        //
        // 3d gaussian distribution is spherically-symmetric so we use that.
        glm::vec4 vec;
        do {
            vec = glm::vec4{
                gaussian_dist(urbg_),
                gaussian_dist(urbg_),
                gaussian_dist(urbg_),
                0.0f
            };
            vec.z = glm::abs(vec.z);
            vec   = glm::normalize(vec);
        } while (glm::dot(glm::vec3{ vec }, glm::vec3{ 0.f, 0.f, 1.f }) < glm::sin(min_sample_angle_rad_));

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

        return vec;
    };

    std::span<glm::vec4> mapped = sampling_kernel_->map_for_write({
        .pending_ops       = PendingOperations::SynchronizeOnMap,
        .flush_policy      = FlushPolicy::AutomaticOnUnmap,
        .previous_contents = PreviousContents::InvalidateAll
    });

    // TODO: Now this is actually very problematic, because the driver
    // sometimes just keeps failing unmapping if there's a bug elsewhere.
    // We need a construct that terminates after a number of tries instead.
    do {

        std::ranges::generate(mapped, hemispherical_vec);

    } while (!sampling_kernel_->unmap_current());

}




inline void SSAO::regenerate_noise_texture() {
    std::normal_distribution<float> gaussian_dist;

    // TODO: We can skip this allocation by mapping a pixel unpack buffer,
    // but there's no implementation of it in GLBuffers. One day...
    PixelData<pixel::RGBF> noise_image{ Size2S(noise_size_) };

    auto random_vec = [&]() -> pixel::RGBF {
        // We really don't care about magnitude since we just orthonormalize
        // this vector in the shader. So no need to normalize here.
        return {
            gaussian_dist(urbg_),
            gaussian_dist(urbg_),
            gaussian_dist(urbg_)
        };
    };

    std::ranges::generate(noise_image, random_vec);


    if (noise_texture_->get_resolution() != noise_size_) {
        noise_texture_ = allocate_texture<TextureTarget::Texture2D>(noise_size_, InternalFormat::RGB16F);
    }

    noise_texture_->upload_image_region({ {}, noise_size_ }, noise_image.data());

    // TODO: I wonder if setting filtering to linear and offseting differently
    // each repeat will produce better results?
    noise_texture_->set_sampler_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
    noise_texture_->set_sampler_wrap_all(Wrap::Repeat);

}




inline void SSAO::operator()(
    RenderEnginePrimaryInterface& engine)
{
    auto* gbuffer = engine.belt().try_get<GBuffer>();

    if (not gbuffer) return;
    if (not enable_occlusion_sampling) return;

    Size2I source_resolution = gbuffer->resolution();
    Size2I target_resolution = scaled_resolution(source_resolution);

    noisy_target_  .resize(target_resolution);
    blurred_target_.resize(target_resolution);

    glapi::set_viewport({ {}, target_resolution });


    // This is inverse of "noise scale" from learnopengl.
    // This is the size of a noise texture in uv coordinates of the screen
    // assuming the size of a pixel is the same for both.
    glm::vec2 noise_size{
        float(noise_size_.width)  / float(source_resolution.width),
        float(noise_size_.height) / float(source_resolution.height),
    };


    // Sampling pass.
    {
        const RawProgram<> sp = sp_sampling_;
        BindGuard bound_camera = engine.bind_camera_ubo();

        gbuffer->depth_texture()        .bind_to_texture_unit(0);
        gbuffer->normals_texture()      .bind_to_texture_unit(1);
        MultibindGuard bound_gbuffer_samplers{
            target_sampler_             ->bind_to_texture_unit(0),
            target_sampler_             ->bind_to_texture_unit(1)
        };

        noise_texture_->bind_to_texture_unit(2);

        sp.uniform("tex_depth",   0);
        sp.uniform("tex_normals", 1);
        sp.uniform("tex_noise",   2);

        sp.uniform("radius",      radius);
        sp.uniform("bias",        bias);
        sp.uniform("noise_size",  noise_size);
        sp.uniform("noise_mode",  to_underlying(noise_mode));


        sampling_kernel_->bind_to_index<BufferTargetIndexed::ShaderStorage>(0);

        {
            BindGuard bound_program = sp.use();
            BindGuard bound_fbo     = noisy_target_.bind_draw();

            glapi::clear_color_buffer(bound_fbo, 0, RGBAF{ .r=0.f });

            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
        }

    }


    // Blur pass.
    {
        const RawProgram<> sp = sp_blur_;
        sp.uniform("noisy_occlusion", 0);
        noisy_target_.color_attachment().texture()         .bind_to_texture_unit(0);
        BindGuard bound_noisy_ao_sampler = target_sampler_->bind_to_texture_unit(0);
        {
            BindGuard bound_program = sp.use();
            BindGuard bound_fbo     = blurred_target_.bind_draw();

            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
        }

    }

    // Update output.
    output_->noisy_texture   = noisy_target_  .color_attachment().texture();
    output_->blurred_texture = blurred_target_.color_attachment().texture();

    // TODO: This is not my responsibility, but everyone else suffers if I don't do this ;_;
    glapi::set_viewport({ {}, source_resolution });

    engine.belt().put_ref(*output_);
}




} // namespace josh::stages::primary
