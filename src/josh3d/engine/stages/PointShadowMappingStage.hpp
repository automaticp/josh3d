#pragma once
#include "GLShaders.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>


namespace josh {


struct PointShadowMaps {
    using PointMapsTarget = RenderTarget<UniqueAttachment<RawCubemapArray>>;
private:
    static PointMapsTarget make_point_maps_target(const Size3I& init_size) {
        using enum GLenum;
        PointMapsTarget tgt{
            { init_size, { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT } }
        };
        tgt.depth_attachment().texture().bind()
            .set_wrap_str(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE)
            .set_min_mag_filters(GL_LINEAR, GL_LINEAR)
            // Enable shadow sampling with built-in 2x2 PCF
            .set_parameter(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE)
            // Comparison: result = ref OPERATOR texture
            // This will return "how much this fragment is lit" from 0 to 1.
            // If you want "how much it's in shadow", use (1.0 - result).
            // Or set the comparison func to GL_GREATER.
            .set_parameter(GL_TEXTURE_COMPARE_FUNC, GL_LESS)
            .unbind();

        return tgt;
    }
public:
    PointMapsTarget point_shadow_maps_tgt{ make_point_maps_target(Size3I{ 1024, 1024, 0 }) };
    // TODO: Life would be easier if this was per-light property.
    glm::vec2 z_near_far{ 0.05f, 150.f };
};


class PointShadowMappingStage {
private:
    SharedStorage<PointShadowMaps> output_;

    ShaderProgram sp_with_alpha{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_cubemap.vert"))
            .load_geom(VPath("src/shaders/depth_cubemap_array.geom"))
            .load_frag(VPath("src/shaders/depth_cubemap.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    ShaderProgram sp_no_alpha{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_cubemap.vert"))
            .load_geom(VPath("src/shaders/depth_cubemap_array.geom"))
            .load_frag(VPath("src/shaders/depth_cubemap.frag"))
            .get()
    };


public:
    PointShadowMappingStage() = default;

    void operator()(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

    SharedStorageView<PointShadowMaps> view_output() const noexcept {
        return output_.share_view();
    }

    void resize_maps(const Size2I& new_size);

    glm::vec2&       z_near_far()       noexcept { return output_->z_near_far; }
    const glm::vec2& z_near_far() const noexcept { return output_->z_near_far; }

private:
    void resize_cubemap_array_storage_if_needed(
        const entt::registry& registry);

    void map_point_shadows(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

};




inline void PointShadowMappingStage::resize_maps(const Size2I& new_size) {
    auto& maps_tgt = output_->point_shadow_maps_tgt;
    maps_tgt.resize_all(new_size);
}


} // namespace josh
