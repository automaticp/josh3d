#pragma once
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLMutability.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "Attachments.hpp"
#include "GLTextures.hpp"
#include "RenderTarget.hpp"
#include "SwapChain.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
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
public:
    bool      use_bloom{ true };
    glm::vec2 threshold_bounds{ 0.05f, 1.0f };
    size_t    blur_iterations{ 1 };
    float     offset_scale{ 1.f };

    float  gaussian_sample_range{ 3.13f }; // TODO: This should be a set/get pair
    size_t gaussian_samples{ 2 };          // so that the buffer update would happen in-place.

    Bloom(const Size2I& initial_resolution);

    void operator()(RenderEnginePostprocessInterface& engine);

    RawTexture2D<GLConst> blur_texture() const noexcept {
        return blur_chain_.front_target().color_attachment().texture();
    }

    Size2I blur_texture_resolution() const noexcept {
        return blur_chain_.front_target().color_attachment().resolution();
    }




private:
    UniqueProgram sp_extract_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_bloom_threshold_extract.frag"))
            .get()
    };

    UniqueProgram sp_twopass_gaussian_blur_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_bloom_twopass_gaussian_blur.frag"))
            .get()
    };

    UniqueProgram sp_blend_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/postprocess.vert"))
            .load_frag(VPath("src/shaders/pp_bloom_blend.frag"))
            .get()
    };

    using BlurTarget    = RenderTarget<NoDepthAttachment, UniqueAttachment<Renderable::Texture2D>>;
    using BlurSwapChain = SwapChain<BlurTarget>;

    UniqueSampler sampler_{ []() {
        UniqueSampler s;
        s->set_min_mag_filters(MinFilter::Linear, MagFilter::Linear);
        s->set_wrap_all(Wrap::ClampToEdge);
        return s;
    }() };

    static BlurTarget make_blur_target(const Size2I& resolution) {
        return {
            resolution,
            { InternalFormat::RGBA16F } // HDR Color
        };
    }

    BlurSwapChain blur_chain_;

    UniqueBuffer<float> weights_buf_;

    float  old_gaussian_sample_range_{ 1.f };
    size_t old_gaussian_samples_{ 0 }; // Must be different from the gaussian_samples on costruction...
    // Above is dumb.

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




inline Bloom::Bloom(const Size2I& initial_resolution)
    : blur_chain_{
        make_blur_target(initial_resolution),
        make_blur_target(initial_resolution)
    }
{
    update_gaussian_blur_weights_if_needed();
}




inline void Bloom::operator()(
    RenderEnginePostprocessInterface& engine)
{

    if (!use_bloom) { return; }


    blur_chain_.resize(engine.main_resolution());


    update_gaussian_blur_weights_if_needed();


    MultibindGuard bound_samplers{
        sampler_->bind_to_texture_unit(0),
        sampler_->bind_to_texture_unit(1)
    };


    // Extract.
    {
        engine.screen_color().bind_to_texture_unit(0);
        sp_extract_->uniform("screen_color",     0);
        sp_extract_->uniform("threshold_bounds", threshold_bounds);
        blur_chain_.draw_and_swap([&, this](auto bound_fbo) {
            BindGuard bound_program = sp_extract_->use();
            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
        });
    }


    // Blur.
    {
        weights_buf_->bind_to_index<BufferTargetIndexed::ShaderStorage>(0);
        sp_twopass_gaussian_blur_->uniform("offset_scale", offset_scale);
        sp_twopass_gaussian_blur_->uniform("screen_color", 0); // Same unit, different textures.

        BindGuard bound_program = sp_twopass_gaussian_blur_->use();

        for (size_t i{ 0 }; i < (2 * blur_iterations); ++i) {
            // Need to rebind after every swap.
            blur_chain_.front_target().color_attachment().texture().bind_to_texture_unit(0);
            sp_twopass_gaussian_blur_->uniform("blur_horizontally", bool(i % 2));

            blur_chain_.draw_and_swap([&](auto bound_fbo) {
                engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
            });
        }
    }


    // Blend.
    // TODO: Why is this a separate shader and not just using blend mode?
    {
        engine.screen_color().bind_to_texture_unit(0);
        blur_chain_.front_target().color_attachment().texture().bind_to_texture_unit(1);
        sp_blend_->uniform("screen_color", 0);
        sp_blend_->uniform("bloom_color",  1);
        {
            BindGuard bound_program = sp_blend_->use();
            engine.draw(bound_program);
        }
    }

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

    auto update_weights = [this](RawBuffer<float> buf) {
        std::span<float> mapped = buf.map_for_write();

        do {
            auto generated =
                detail::generate_binned_gaussian_no_tails(
                    -gaussian_sample_range, gaussian_sample_range,
                    gaussian_weights_buffer_size()
                );

            std::ranges::copy(generated, mapped.begin());

        } while (!buf.unmap_current());
    };


    if (gaussian_weights_buffer_needs_resizing()) {

        resize_to_fit(weights_buf_, NumElems{ gaussian_weights_buffer_size() });
        update_weights(weights_buf_);

    } else if (gaussian_weight_values_need_updating()) {

        update_weights(weights_buf_);

    }


    old_gaussian_sample_range_ = gaussian_sample_range;
    old_gaussian_samples_      = gaussian_samples;

}




} // namespace josh::stages::postprocess
