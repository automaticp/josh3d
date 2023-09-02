#pragma once
#include "GLShaders.hpp"
#include "RenderEngine.hpp"
#include "RenderTargetDepthCubemapArray.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>


namespace josh {


struct PointShadowMaps {
    RenderTargetDepthCubemapArray point_shadow_maps{ Size3I{ 1024, 1024, 0 } };
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
    PointShadowMappingStage() {
        using enum GLenum;
        output_->point_shadow_maps.depth_taget()
            .bind()
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            // Enable shadow sampling with built-in 2x2 PCF
            .set_parameter(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE)
            // Comparison: result = ref OPERATOR texture
            // This will return "how much this fragment is lit" from 0 to 1.
            // If you want "how much it's in shadow", use (1.0 - result).
            // Or set the comparison func to GL_GREATER.
            .set_parameter(GL_TEXTURE_COMPARE_FUNC, GL_LESS)
            .unbind();
    }

    void operator()(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

    SharedStorageView<PointShadowMaps> view_output() const noexcept {
        return output_.share_view();
    }

    void resize_maps(Size2I new_size);

    glm::vec2&       z_near_far()       noexcept { return output_->z_near_far; }
    const glm::vec2& z_near_far() const noexcept { return output_->z_near_far; }

private:
    void resize_cubemap_array_storage_if_needed(
        const entt::registry& registry);

    void map_point_shadows(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

};




inline void PointShadowMappingStage::resize_maps(Size2I new_size) {
    auto& maps = output_->point_shadow_maps;
    maps.reset_size(Size3I{ new_size, maps.size().depth });
}


} // namespace josh
