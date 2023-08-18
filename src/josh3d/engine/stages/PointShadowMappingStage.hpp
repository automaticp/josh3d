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
    RenderTargetDepthCubemapArray point_shadow_maps{ 1024, 1024, 0 };
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
    void operator()(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

    SharedStorageView<PointShadowMaps> view_output() const noexcept {
        return output_.share_view();
    }

    void resize_maps(GLsizei width, GLsizei height);

private:
    void resize_cubemap_array_storage_if_needed(
        const entt::registry& registry);

    void map_point_shadows(
        const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry);

};




inline void PointShadowMappingStage::resize_maps(
    GLsizei width, GLsizei height)
{
    auto& maps = output_->point_shadow_maps;
    maps.reset_size(width, height, maps.depth());
}


} // namespace josh
