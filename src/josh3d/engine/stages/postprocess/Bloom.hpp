#pragma once
#include "DefaultResources.hpp"
#include "GLBuffers.hpp"
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "Attachments.hpp"
#include "GLTextures.hpp"
#include "RenderTarget.hpp"
#include "SwapChain.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Size.hpp"
#include "VPath.hpp"
#include <glbinding/gl/enum.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <cmath>
#include <range/v3/view/generate_n.hpp>
#include <cassert>
#include <cstddef>




namespace josh::stages::postprocess {


class Bloom {
private:
    UniqueShaderProgram sp_extract_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_bloom_threshold_extract.frag"))
            .get()
    };

    UniqueShaderProgram sp_twopass_gaussian_blur_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_bloom_twopass_gaussian_blur.frag"))
            .get()
    };

    UniqueShaderProgram sp_blend_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_bloom_blend.frag"))
            .get()
    };

    using BlurTarget    = RenderTarget<NoDepthAttachment, UniqueAttachment<RawTexture2D>>;
    using BlurSwapChain = SwapChain<BlurTarget>;

    static BlurTarget make_blur_target() {
        using enum GLenum;
        BlurTarget tgt{
            {},                                // No Depth
            { Size2I{ 0, 0 }, { GL_RGBA16F } } // HDR Color
        };
        // TODO: That's one way to do it.
        // Another one would be to support Sampler objects.
        tgt.color_attachment().texture()
            .bind()
            .set_min_mag_filters(GL_LINEAR, GL_LINEAR)
            .set_wrap_st(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER)
            .unbind();

        return tgt;
    }

    BlurSwapChain blur_chain_{
        make_blur_target(), make_blur_target()
    };

    UniqueSSBO weights_ssbo_;
    float  old_gaussian_sample_range_{ 1.f };
    size_t old_gaussian_samples_{ 0 }; // Must be different from the gaussian_samples on costruction...

public:
    glm::vec2 threshold_bounds{ 0.05f, 1.0f };
    size_t    blur_iterations{ 1 };
    float     offset_scale{ 1.f };
    bool      use_bloom{ true };

    float  gaussian_sample_range{ 1.8f };
    size_t gaussian_samples{ 4 };

    Bloom();

    void operator()(
        const RenderEnginePostprocessInterface& engine,
        const entt::registry&);

    RawTexture2D<GLConst> blur_texture() const noexcept {
        return blur_chain_.front_target().color_attachment().texture();
    }

    Size2I blur_texture_size() const noexcept {
        return blur_chain_.front_target().color_attachment().size();
    }

private:
    // From -x to +x binned into 2 * n_samples + 1.
    void update_gaussian_blur_weights_if_needed();

    size_t gaussian_weights_buffer_size() const noexcept {
        return (gaussian_samples * 2) + 1;
    }

    bool gaussian_weights_buffer_needs_resizing() const noexcept {
        return gaussian_samples != old_gaussian_samples_;
    }

    bool gaussian_weight_values_need_updating() const noexcept {
        return gaussian_sample_range != old_gaussian_sample_range_;
    }

};




inline Bloom::Bloom() {
    update_gaussian_blur_weights_if_needed();
}




inline void Bloom::operator()(
    const RenderEnginePostprocessInterface& engine, const entt::registry&)
{

    if (!use_bloom) { return; }

    if (engine.window_size() != blur_chain_.back_target().color_attachment().size()) {
        // TODO: Might be part of Attachment::resize() to skip redundant resizes
        blur_chain_.resize_all(engine.window_size());
    }

    update_gaussian_blur_weights_if_needed();


    // Extract
    blur_chain_.draw_and_swap([&, this] {
        sp_extract_.use().and_then([&, this](ActiveShaderProgram<GLMutable>& ashp) {

            ashp.uniform("threshold_bounds", threshold_bounds)
                .uniform("screen_color", 0);
            engine.screen_color().bind_to_unit_index(0);

            globals::quad_primitive_mesh().draw();
        });
    });

    // Blur
    weights_ssbo_.bind_to_index(0).and_then([&, this] {

        for (size_t i{ 0 }; i < (2 * blur_iterations); ++i) {
            blur_chain_.draw_and_swap([&, this] {
                sp_twopass_gaussian_blur_.use().and_then([&, this](
                    ActiveShaderProgram<GLMutable>& ashp)
                {

                    ashp.uniform("blur_horizontally", bool(i % 2))
                        .uniform("offset_scale",      offset_scale)
                        .uniform("screen_color",      0);
                    blur_chain_.front_target()
                        .color_attachment()
                        .texture()
                        .bind_to_unit_index(0);

                    globals::quad_primitive_mesh().draw();
                });
            });
        }

    })
    .unbind();

    // Blend
    sp_blend_.use().and_then([&, this](ActiveShaderProgram<GLMutable>& ashp) {

        ashp.uniform("screen_color", 0)
            .uniform("bloom_color",  1);
        engine.screen_color().bind_to_unit_index(0);
        blur_chain_.front_target().color_attachment().texture().bind_to_unit_index(1);

        engine.draw();

    });

}




namespace detail {


inline float gaussian_cdf(float x) noexcept {
    return (1.f + std::erf(x / std::sqrt(2.f))) / 2.f;
}


// Uniformly bins the normal distribution from x0 to x1.
// Does not preserve the sum as the tails are not accounted for.
// Accounting for tails can make them biased during sampling.
// Does not normalize the resulting bins.
inline auto generate_binned_gaussian_no_tails(
    float from, float to, size_t n_bins) noexcept
{
    assert(to > from);

    const float step{ (to - from) / float(n_bins) };
    float current_x{ from };
    float previous_cdf = gaussian_cdf(from);

    return ranges::views::generate_n(
        [=]() mutable {
            current_x += step;
            const float current_cdf{ gaussian_cdf(current_x) };
            const float diff = current_cdf - previous_cdf;
            previous_cdf = current_cdf;
            return diff;
        },
        n_bins
    );
}


} // namespace detail




inline void Bloom::update_gaussian_blur_weights_if_needed() {
    // FIXME: The weights are not normalized over the range of x
    // leading to a noticable loss of color yield when
    // the range is too high. Is this okay?

    using enum GLenum;

    auto update_weights = [this](BoundIndexedSSBO<>& ssbo) {
        std::span<float> mapped =
            ssbo.map_for_write<float>(0, gaussian_weights_buffer_size());

        auto generated =
            detail::generate_binned_gaussian_no_tails(
                -gaussian_sample_range, gaussian_sample_range,
                gaussian_weights_buffer_size()
            );

        std::ranges::copy(generated, mapped.begin());

        ssbo.unmap_current();
    };


    if (gaussian_weights_buffer_needs_resizing()) {

        weights_ssbo_.bind_to_index(0)
            .allocate_data<float>(gaussian_weights_buffer_size(), GL_STATIC_DRAW)
            .and_then(update_weights)
            .unbind();

    } else if (gaussian_weight_values_need_updating()) {

        weights_ssbo_.bind_to_index(0)
            .and_then(update_weights)
            .unbind();

    }


    old_gaussian_sample_range_ = gaussian_sample_range;
    old_gaussian_samples_      = gaussian_samples;

}




} // namespace josh::stages::postprocess
