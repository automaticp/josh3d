#include "BloomAW.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLProgram.hpp"
#include "GLTextures.hpp"
#include "GLUniformTraits.hpp"
#include "Mesh.hpp"
#include "Region.hpp"
#include "RenderEngine.hpp"


namespace josh {


void BloomAW::operator()(
    RenderEnginePostprocessInterface& engine)
{
    if (not enable_bloom) return;

    // NOTE: Taking half-resolution as the base MIP.
    const auto [w, h] = engine.main_resolution();
    _resize_texture({ w / 2, h / 2 });

    // Put an upper cap on the number of levels.
    const NumLevels max_levels = GLsizei(max_downsample_levels);
    const NumLevels has_levels = GLsizei(num_available_levels());
    const NumLevels num_levels = std::min(max_levels, has_levels);
    const MipLevel  last_lod   = num_levels - 1;

    // Downsample.
    {
        const RawProgram<> sp = sp_downsample_;

        const BindGuard bfb           = fbo_->bind_draw();
        const BindGuard bsp           = sp.use();
        const BindGuard bound_sampler = sampler_->bind_to_texture_unit(0);

        sp.uniform("source", 0);

        // First downsample main texture to the bloom_texture_.

        // Sample from:
        engine.screen_color().bind_to_texture_unit(0);

        // Draw to:
        fbo_->attach_texture_to_color_buffer(bloom_texture_, 0);
        glapi::set_viewport({ {}, bloom_texture_->get_resolution() });

        engine.primitives().quad_mesh().draw(bsp, bfb);

        // Then progressively downsample further.
        bloom_texture_->bind_to_texture_unit(0); // Always bound, but we don't sample overlapping LODs.

        for (MipLevel lod = 0; lod < last_lod; ++lod)
        {
            const MipLevel src_lod        = lod;
            const MipLevel dst_lod        = lod + 1;
            const Extent2I dst_resolution = bloom_texture_->get_resolution(dst_lod);

            // Sample from:
            bloom_texture_->set_base_level(src_lod);
            bloom_texture_->set_max_level (src_lod);

            // NOTE: It is not enough to sample only from a single level
            // in the shader using textureLod(), as this results in UB still
            // (At least on my hardware/driver configuration).
            // Restricting the range of LOD levels accessible to the shader
            // works better in this case.

            // Draw to:
            fbo_->attach_texture_to_color_buffer(bloom_texture_, 0, dst_lod);

            // NOTE: LOD level for attaching a texture is view/storage level,
            // and is not controlled by lod_base and lod_max.

            glapi::set_viewport({ {}, dst_resolution });

            engine.primitives().quad_mesh().draw(bsp, bfb);
        }
    }

    // Upsample.
    {
        const RawProgram<> sp = sp_upsample_;

        const BindGuard bfb           = fbo_->bind_draw();
        const BindGuard bsp           = sp.use();
        const BindGuard bound_sampler = sampler_->bind_to_texture_unit(0);

        sp.uniform("source",          0);
        sp.uniform("filter_scale_px", filter_scale_px);

        glapi::enable(Capability::Blending);
        glapi::set_blend_factors(BlendFactor::One, BlendFactor::One);
        glapi::set_blend_equation(BlendEquation::FactorAdd);

        bloom_texture_->bind_to_texture_unit(0);

        for (MipLevel lod = last_lod; lod > 0; --lod)
        {
            const MipLevel src_lod        = lod;
            const MipLevel dst_lod        = lod - 1;
            const Size2I   dst_resolution = bloom_texture_->get_resolution(dst_lod);

            // Sample from:
            bloom_texture_->set_base_level(src_lod);
            bloom_texture_->set_max_level (src_lod);

            // Draw to:
            fbo_->attach_texture_to_color_buffer(bloom_texture_, 0, dst_lod);

            glapi::set_viewport({ {}, dst_resolution });

            engine.primitives().quad_mesh().draw(bsp, bfb);
        }

        fbo_->detach_color_buffer(0);
        glapi::set_blend_factors(BlendFactor::One, BlendFactor::OneMinusSrcAlpha);
        glapi::disable(Capability::Blending);
    }

    // Apply to the main buffer.
    {
        const RawProgram<> sp = sp_apply_;

        const BindGuard bsp = sp.use();
        const MultibindGuard bound_samplers = {
            screen_sampler_->bind_to_texture_unit(0),
            sampler_       ->bind_to_texture_unit(1),
        };

        engine.screen_color().bind_to_texture_unit(0);
        bloom_texture_      ->bind_to_texture_unit(1);
        bloom_texture_->set_base_level(0);
        bloom_texture_->set_max_level (0);

        sp.uniform("screen_color", 0);
        sp.uniform("bloom_color",  1);
        sp.uniform("bloom_weight", bloom_weight);

        glapi::set_viewport({ {}, engine.main_resolution() });

        engine.draw(bsp);
    }
}

auto BloomAW::num_available_levels() const noexcept
    -> usize
{
    return bloom_texture_->get_num_storage_levels();
}

void BloomAW::_resize_texture(Extent2I new_resolution)
{
    if (new_resolution != bloom_texture_->get_resolution())
    {
        bloom_texture_ = {};
        bloom_texture_->allocate_storage(new_resolution, InternalFormat::R11F_G11F_B10F, max_num_levels(new_resolution));
    }
}




} // namespace josh
