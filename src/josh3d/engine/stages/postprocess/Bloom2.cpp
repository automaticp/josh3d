#include "Bloom2.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLProgram.hpp"
#include "GLTextures.hpp"
#include "GLUniformTraits.hpp"
#include "Mesh.hpp"
#include "RenderEngine.hpp"
#include <cstddef>


namespace josh::stages::postprocess {
namespace {


void resize_bloom_texture(
    UniqueTexture2D& texture,
    const Size2I&    new_resolution)
{
    const Size2I old_resolution = texture->get_resolution();

    if (old_resolution != new_resolution) {
        if (old_resolution.width != 0) {
            // Need to discard previous object.
            texture = {};
        }

        texture->allocate_storage(new_resolution, InternalFormat::R11F_G11F_B10F, max_num_levels(new_resolution));
    }
}


} // namespace




auto Bloom2::num_available_levels() const noexcept
    -> size_t
{
    return bloom_texture_->get_num_storage_levels();
}


void Bloom2::operator()(
    RenderEnginePostprocessInterface& engine)
{
    if (!enable_bloom) { return; }

    const Size2I main_resolution = engine.main_resolution();
    resize_bloom_texture(bloom_texture_, { main_resolution.width / 2, main_resolution.height / 2 });

    // Put an upper cap on the number of levels.
    const NumLevels max_levels = GLsizei(max_downsample_levels);
    const NumLevels has_levels = GLsizei(num_available_levels());
    const NumLevels num_levels = std::min(max_levels, has_levels);
    const MipLevel  last_lod   = num_levels - 1;

    // Downsample.
    {
        const RawProgram<> sp = sp_downsample_;

        BindGuard bound_fbo     = fbo_->bind_draw();
        BindGuard bound_program = sp.use();
        BindGuard bound_sampler = sampler_->bind_to_texture_unit(0);

        sp.uniform("source", 0);

        // First downsample main texture to the bloom_texture_.

        // Sample from:
        engine.screen_color().bind_to_texture_unit(0);

        // Draw to:
        fbo_->attach_texture_to_color_buffer(bloom_texture_, 0);
        glapi::set_viewport({ {}, bloom_texture_->get_resolution() });

        engine.primitives().quad_mesh().draw(bound_program, bound_fbo);


        // Then progressively downsample further.
        bloom_texture_->bind_to_texture_unit(0); // Always bound, but we don't sample overlapping LODs.

        for (MipLevel lod{ 0 }; lod < last_lod; ++lod) {
            const MipLevel src_lod        = lod;
            const MipLevel dst_lod        = lod + 1;
            const Size2I   dst_resolution = bloom_texture_->get_resolution(dst_lod);

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

            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
        }
    }

    // Upsample.
    {
        const RawProgram<> sp = sp_upsample_;

        BindGuard bound_fbo     = fbo_->bind_draw();
        BindGuard bound_program = sp.use();
        BindGuard bound_sampler = sampler_->bind_to_texture_unit(0);

        sp.uniform("source", 0);
        sp.uniform("filter_scale_px", filter_scale_px);

        glapi::enable(Capability::Blending);
        glapi::set_blend_factors(BlendFactor::One, BlendFactor::One);
        glapi::set_blend_equation(BlendEquation::FactorAdd);


        bloom_texture_->bind_to_texture_unit(0);

        for (MipLevel lod = last_lod; lod > 0; --lod) {
            const MipLevel src_lod        = lod;
            const MipLevel dst_lod        = lod - 1;
            const Size2I   dst_resolution = bloom_texture_->get_resolution(dst_lod);

            // Sample from:
            bloom_texture_->set_base_level(src_lod);
            bloom_texture_->set_max_level (src_lod);

            // Draw to:
            fbo_->attach_texture_to_color_buffer(bloom_texture_, 0, dst_lod);

            glapi::set_viewport({ {}, dst_resolution });

            engine.primitives().quad_mesh().draw(bound_program, bound_fbo);
        }

        fbo_->detach_color_buffer(0);
        glapi::set_blend_factors(BlendFactor::One, BlendFactor::OneMinusSrcAlpha);
        glapi::disable(Capability::Blending);
    }


    // Apply to the main buffer.
    {
        const RawProgram<> sp = sp_apply_;

        BindGuard bound_program = sp.use();
        MultibindGuard bound_samplers{
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

        glapi::set_viewport({ {}, main_resolution });

        engine.draw(bound_program);
    }

}



} // namespace josh::stages::postprocess
