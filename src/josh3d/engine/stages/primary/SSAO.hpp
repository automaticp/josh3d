#pragma once
#include "Attachments.hpp"
#include "DefaultResources.hpp"
#include "EnumUtils.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "ImageData.hpp"
#include "PixelPackTraits.hpp" // IWYU pragma: keep (traits)
#include "UniformTraits.hpp"   // IWYU pragma: keep (traits)
#include "Pixels.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "ShaderBuilder.hpp"
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

    enum class NoiseMode : GLint {
        SampledFromTexture = 0,
        GeneratedInShader  = 1
    } noise_mode{ NoiseMode::GeneratedInShader };

    enum class PositionSource : GLint {
        GBuffer = 0,
        Depth   = 1
    } position_source{ PositionSource::Depth };


    SSAO(SharedStorageView<GBuffer> gbuffer);

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
    UniqueProgram sp_sampling_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ssao_sampling.frag"))
            .get()
    };

    UniqueProgram sp_blur_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ssao_blur.frag"))
            .get()
    };


    SharedStorageView<GBuffer> gbuffer_;

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
        s->set_min_mag_filters(MinFilter::Nearest, MagFilter::Nearest);
        s->set_wrap_all(Wrap::ClampToEdge);
        return s;
    }() };

    UniqueTexture2D noise_texture_;
    Size2I noise_size_{ 16, 16 };


    // It's still unclear to me why they call it "kernel".
    // I get that it *sounds like* convolution:
    //   "for each pixel get N samples and reduce that to one value"
    // But it is not convolution in a mathematical sense, no?


    // We use vec4 to avoid issues with alignment in std430,
    // even though we only need vec3 of data.
    UniqueBuffer<glm::vec4> sampling_kernel_{
        allocate_buffer<glm::vec4>(NumElems{ 9 }, StorageMode::StaticServer)
    };

    float min_sample_angle_rad_{ glm::radians(5.f) };


    inline static thread_local std::mt19937 urbg_{};

};




inline SSAO::SSAO(SharedStorageView<GBuffer> gbuffer)
    : gbuffer_{ std::move(gbuffer) }
    , noisy_target_{
        gbuffer_->resolution(),
        { InternalFormat::R8 }
    }
    , blurred_target_{
        gbuffer_->resolution(),
        { InternalFormat::R8 }
    }
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

    std::span<glm::vec4> mapped = sampling_kernel_->map_for_write(
        PendingOperations::SynchronizeOnMap,
        FlushPolicy::AutomaticOnUnmap,
        PreviousContents::InvalidateAll
    );

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
    ImageData<pixel::RGBF> noise_image{ Size2S(noise_size_) };

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

    if (!enable_occlusion_sampling) return;

    Size2I resolution = gbuffer_->resolution();

    noisy_target_  .resize(resolution);
    blurred_target_.resize(resolution);

    auto& cam_params = engine.camera().get_params();

    // This is inverse of "noise scale" from learnopengl.
    // This is the size of a noise texture in uv coordinates of the screen
    // assuming the size of a pixel is the same for both.
    glm::vec2 noise_size{
        float(noise_size_.width)  / float(resolution.width),
        float(noise_size_.height) / float(resolution.height),
    };

    glm::mat4 view_mat    = engine.camera().view_mat();
    glm::mat3 normal_view = glm::transpose(glm::inverse(view_mat));
    glm::mat4 proj_mat    = engine.camera().projection_mat();
    glm::mat4 inv_proj    = glm::inverse(proj_mat);


    // Sampling pass.

    sp_sampling_->uniform("view",        view_mat);
    sp_sampling_->uniform("normal_view", normal_view);
    sp_sampling_->uniform("proj",        proj_mat);
    sp_sampling_->uniform("inv_proj",    inv_proj);
    sp_sampling_->uniform("z_near",      cam_params.z_near);
    sp_sampling_->uniform("z_far",       cam_params.z_far);
    sp_sampling_->uniform("radius",      radius);
    sp_sampling_->uniform("bias",        bias);
    sp_sampling_->uniform("tex_position_draw", 0);
    sp_sampling_->uniform("tex_normals",       1);
    sp_sampling_->uniform("tex_depth",         2);
    sp_sampling_->uniform("tex_noise",         3);
    sp_sampling_->uniform("noise_size",        noise_size);
    sp_sampling_->uniform("noise_mode",        to_underlying(noise_mode));
    sp_sampling_->uniform("position_source",   to_underlying(position_source));


    gbuffer_->position_draw_texture().bind_to_texture_unit(0);
    gbuffer_->normals_texture()      .bind_to_texture_unit(1);
    gbuffer_->depth_texture()        .bind_to_texture_unit(2);
    target_sampler_->bind_to_texture_unit(0);
    target_sampler_->bind_to_texture_unit(1);
    target_sampler_->bind_to_texture_unit(2);


    noise_texture_->bind_to_texture_unit(3);
    unbind_sampler_from_unit(3);


    sampling_kernel_->bind_to_index<BufferTargetIndexed::ShaderStorage>(0);

    {
        auto bound_program = sp_sampling_->use();
        auto bound_fbo     = noisy_target_.bind_draw();


        glapi::clear_color_buffer(bound_fbo, 0, RGBAF{ .r=0.f });

        globals::quad_primitive_mesh().draw(bound_program, bound_fbo);


        bound_fbo.unbind();
        bound_program.unbind();
    }

    unbind_samplers_from_units(0, 1, 2);




    // Blur pass.

    sp_blur_->uniform("noisy_occlusion", 0);
    noisy_target_.color_attachment().texture().bind_to_texture_unit(0);
    target_sampler_->bind_to_texture_unit(0);
    {
        auto bound_program = sp_blur_->use();
        auto bound_fbo     = blurred_target_.bind_draw();

        globals::quad_primitive_mesh().draw(bound_program, bound_fbo);

        bound_fbo.unbind();
        bound_program.unbind();
    }

    unbind_sampler_from_unit(0);


    // Update output.
    output_->noisy_texture   = noisy_target_.color_attachment().texture();
    output_->blurred_texture = blurred_target_.color_attachment().texture();
}




} // namespace josh::stages::primary
