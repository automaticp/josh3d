#include "GaussianBloom.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "Scalars.hpp"
#include "ShaderPool.hpp"
#include "UniformTraits.hpp"
#include "RenderEngine.hpp"
#include "Region.hpp"
#include <range/v3/view/generate_n.hpp>
#include <cmath>
#include <cassert>
#include <ranges>



namespace josh {


GaussianBloom::GaussianBloom(usize kernel_limb_size, float kernel_range)
{
    resize_kernel(kernel_limb_size, kernel_range);
}

void GaussianBloom::operator()(
    RenderEnginePostprocessInterface& engine)
{
    if (not use_bloom) return;

    target._resize(engine.main_resolution());

    const MultibindGuard bound_samplers = {
        sampler_->bind_to_texture_unit(0),
        sampler_->bind_to_texture_unit(1)
    };

    // Extract.
    {
        const auto sp = sp_extract_.get();
        engine.screen_color().bind_to_texture_unit(0);
        sp.uniform("screen_color",     0);
        sp.uniform("threshold_bounds", threshold_bounds);
        const BindGuard bsp = sp.use();
        const BindGuard bfb = target._back().fbo->bind_draw();
        engine.primitives().quad_mesh().draw(bsp, bfb);
        target._swap();
    }

    // Blur.
    {
        const auto sp = sp_twopass_gaussian_blur_.get();
        kernel_weights_.bind_to_ssbo_index(0);
        sp.uniform("offset_scale", offset_scale);
        sp.uniform("screen_color", 0); // Same unit, different textures.

        const BindGuard bsp = sp.use();

        for (const uindex i : irange(2 * blur_iterations))
        {
            // Need to rebind after every swap.
            target._front().texture->bind_to_texture_unit(0);
            sp.uniform("blur_horizontally", bool(i % 2));
            const BindGuard bfb = target._back().fbo->bind_draw();
            engine.primitives().quad_mesh().draw(bsp, bfb);
            target._swap();
        }
    }

    // Blend.
    // TODO: Why is this a separate shader and not just using blend mode?
    {
        const auto sp = sp_blend_.get();
        engine.screen_color().bind_to_texture_unit(0);
        target._front().texture->bind_to_texture_unit(1);
        sp.uniform("screen_color", 0);
        sp.uniform("bloom_color",  1);
        const BindGuard bsp = sp.use();
        engine.draw(bsp);
    }
}

namespace {

auto gaussian_cdf(float x) noexcept
    -> float
{
    return (1.f + std::erf(x / std::numbers::sqrt2_v<float>)) / 2.f;
}

/*
Uniformly bins the normal distribution from x0 to x1.
Does not preserve the sum as the tails are not accounted for.
Accounting for tails can make them biased during sampling.
Does not normalize the resulting bins.
*/
auto generate_binned_gaussian_no_tails(
    float from,
    float to,
    usize n_bins) noexcept
        -> std::ranges::input_range auto
{
    assert(to > from);

    const float step = (to - from) / float(n_bins);
    float current_x    = from;
    float previous_cdf = gaussian_cdf(from);

    return ranges::views::generate_n(
        [=]() mutable {
            current_x += step;
            const float current_cdf = gaussian_cdf(current_x);
            const float diff = current_cdf - previous_cdf;
            previous_cdf = current_cdf;
            return diff;
        },
        n_bins
    );
}

} // namespace


auto GaussianBloom::kernel_limb_size() const noexcept
    -> usize
{
    const usize n = kernel_weights_.num_staged();
    if (n == 0) return 0;
    return (n - 1) / 2;
}

void GaussianBloom::resize_kernel(usize limb_size, float range)
{
    const usize new_n = (2 * limb_size) + 1;
    if (new_n != kernel_weights_.num_staged() or
        range != kernel_range_)
    {
        kernel_range_ = range;
        kernel_weights_.restage(generate_binned_gaussian_no_tails(-range, range, new_n));
    }
}


} // namespace josh
