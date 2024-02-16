#pragma once
#include "Attachments.hpp"
#include "DefaultResources.hpp"
#include "EnumUtils.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "ImageData.hpp"
#include "Pixels.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "TextureHelpers.hpp"
#include "VPath.hpp"
#include "GLScalars.hpp"
#include "stages/primary/GBufferStorage.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <span>
#include <random>




namespace josh::stages::primary {


class SSAO {
private:
    UniqueShaderProgram sp_sampling_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ssao_sampling.frag"))
            .get()
    };

    UniqueShaderProgram sp_blur_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/ssao_blur.frag"))
            .get()
    };


    using NoisyTarget = RenderTarget<NoDepthAttachment, UniqueAttachment<RawTexture2D>>;

    NoisyTarget noisy_target_{
        []() -> NoisyTarget {
            using enum GLenum;
            NoisyTarget tgt{
                {},
                { Size2I{ 0, 0 }, { GL_RED } }
            };
            tgt.color_attachment().texture().bind()
                .set_min_mag_filters(GL_NEAREST, GL_NEAREST)
                .set_wrap_st(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE)
                .unbind();
            return tgt;
        }()
    };


    using BlurredTarget = RenderTarget<NoDepthAttachment, UniqueAttachment<RawTexture2D>>;

    BlurredTarget blurred_target_{
        []() -> BlurredTarget {
            using enum GLenum;
            BlurredTarget tgt{
                {},
                { Size2I{ 0, 0 }, { GL_RED } }
            };
            tgt.color_attachment().texture().bind()
                .set_min_mag_filters(GL_NEAREST, GL_NEAREST)
                .set_wrap_st(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE)
                .unbind();
            return tgt;
        }()
    };


    SharedStorageView<GBuffer> gbuffer_;


    UniqueTexture2D noise_texture_;
    Size2I noise_size_{ 16, 16 };


    // It's still unclear to me why they call it "kernel".
    // I get that it *sounds like* convolution:
    //   "for each pixel get N samples and reduce that to one value"
    // But it is not convolution in a mathematical sense, no?
    UniqueSSBO sampling_kernel_;
    GLsizeiptr kernel_size_{ 16 };
    float min_sample_angle_rad_{ glm::radians(5.f) };


    inline static thread_local std::mt19937 urbg_{};
    void regenerate_kernels();
    void regenerate_noise();

public:
    // Switch to false if you want to skip the whole stage.
    bool  enable_occlusion_sampling{ true };
    float radius{ 0.2f };
    float bias{ 0.01f }; // Can't wait to do another reciever plane algo...

    enum class NoiseMode : GLint {
        sampled_from_texture = 0,
        generated_in_shader  = 1
    } noise_mode{ NoiseMode::generated_in_shader };

    enum class PositionSource : GLint {
        gbuffer = 0,
        depth   = 1
    } position_source{ PositionSource::depth };


    SSAO(SharedStorageView<GBuffer> gbuffer);

    size_t get_kernel_size() const noexcept { return kernel_size_; }
    void   set_kernel_size(size_t new_size) { kernel_size_ = new_size; regenerate_kernels(); }

    float get_min_sample_angle_from_surface_rad() const noexcept { return min_sample_angle_rad_; }
    void  set_min_sample_angle_from_surface_rad(float angle_rad) { min_sample_angle_rad_ = angle_rad; regenerate_kernels(); }

    Size2I get_noise_texture_size() const noexcept { return noise_size_; }
    void   set_noise_texture_size(Size2I new_size) { noise_size_ = new_size; regenerate_noise(); }

    auto get_occlusion_texture() const noexcept -> RawTexture2D<GLConst>;
    auto get_noisy_occlusion_texture() const noexcept -> RawTexture2D<GLConst>;

    void operator()(const RenderEnginePrimaryInterface& engine, const entt::registry& registry);
};




inline SSAO::SSAO(SharedStorageView<GBuffer> gbuffer)
    : gbuffer_{ std::move(gbuffer) }
{
    regenerate_kernels();
    regenerate_noise();
}


inline auto SSAO::get_occlusion_texture() const noexcept
    -> RawTexture2D<GLConst>
{
    return blurred_target_.color_attachment().texture();
}


inline auto SSAO::get_noisy_occlusion_texture() const noexcept
    -> RawTexture2D<GLConst>
{
    return noisy_target_.color_attachment().texture();
}


inline void SSAO::regenerate_kernels() {
    std::normal_distribution<float>       gaussian_dist;
    std::uniform_real_distribution<float> uniform_dist;

    // Generate and upload the kernel.
    sampling_kernel_.bind()
        .allocate_data<glm::vec4>(kernel_size_, gl::GL_STATIC_DRAW)
        .and_then([&](BoundSSBO<>& ssbo) {
            // We use vec4 to avoid issues with alignment in std430,
            // even though we only need vec3 of data.
            std::span<glm::vec4> mapped = ssbo.map_for_write<glm::vec4>();

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

            std::ranges::generate(mapped, hemispherical_vec);

            ssbo.unmap_current();
        })
        .unbind();

}




inline void SSAO::regenerate_noise() {
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


    using enum GLenum;

    noise_texture_.bind()
        .and_then([&](BoundTexture2D<>& tex) {
            attach_data_to_texture(tex, noise_image, GL_RGB16F);
        })
        // TODO: I wonder if setting filtering to linear and offseting differently
        // each repeat will produce better results?
        .set_min_mag_filters(GL_NEAREST, GL_NEAREST)
        .set_wrap_st(GL_REPEAT, GL_REPEAT)
        .unbind();

}




inline void SSAO::operator()(
    const RenderEnginePrimaryInterface& engine,
    const entt::registry&)
{
    using enum GLenum;

    if (!enable_occlusion_sampling) return;

    noisy_target_  .resize_all(engine.window_size());
    blurred_target_.resize_all(engine.window_size());

    auto& cam_params = engine.camera().get_params();

    // This is inverse of "noise scale" from learnopengl.
    // This is the size of a noise texture in uv coordinates of the screen
    // assuming the size of a pixel is the same for both.
    glm::vec2 noise_size{
        float(noise_size_.width)  / float(engine.window_size().width),
        float(noise_size_.height) / float(engine.window_size().height),
    };

    glm::mat4 view_mat    = engine.camera().view_mat();
    glm::mat3 normal_view = glm::transpose(glm::inverse(view_mat));
    glm::mat4 proj_mat    = engine.camera().projection_mat();
    glm::mat4 inv_proj    = glm::inverse(proj_mat);

    sp_sampling_.use()
        .uniform("view",        view_mat)
        .uniform("normal_view", normal_view)
        .uniform("proj",        proj_mat)
        .uniform("inv_proj",    inv_proj)
        .uniform("z_near",      cam_params.z_near)
        .uniform("z_far",       cam_params.z_far)
        .uniform("radius",      radius)
        .uniform("bias",        bias)
        .uniform("tex_position_draw", 0)
        .uniform("tex_normals",       1)
        .uniform("tex_depth",         2)
        .uniform("tex_noise",         3)
        .uniform("noise_size",        noise_size)
        .uniform("noise_mode",        to_underlying(noise_mode))
        .uniform("position_source",   to_underlying(position_source))
        .and_then([this] {
            // Ideally, we should use custom sampler objects here
            // to guarantee that we are sampling in CLAMP_TO_EDGE
            // mode, independent of what the GBuffer sets,
            // but I'm lazy, oh well...
            gbuffer_->position_draw_texture().bind_to_unit_index(0);
            gbuffer_->normals_texture()      .bind_to_unit_index(1);
            gbuffer_->depth_texture()        .bind_to_unit_index(2);
            noise_texture_                   .bind_to_unit_index(3);
            sampling_kernel_.bind_to_index(0);

            noisy_target_.bind_draw().and_then([] {
                gl::glClear(gl::GL_COLOR_BUFFER_BIT);
                globals::quad_primitive_mesh().draw();
            }).unbind();
        });

    sp_blur_.use()
        .uniform("noisy_occlusion", 0)
        .and_then([this] {
            noisy_target_.color_attachment().texture().bind_to_unit_index(0);

            blurred_target_.bind_draw().and_then([] {
                globals::quad_primitive_mesh().draw();
            }).unbind();
        });

}




} // namespace josh::stages::primary
