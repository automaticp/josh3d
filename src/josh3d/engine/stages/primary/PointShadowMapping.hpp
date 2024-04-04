#pragma once
#include "GLAPIBinding.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "ShaderBuilder.hpp"
#include "SharedStorage.hpp"
#include "GLObjects.hpp"
#include "VPath.hpp"
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/enum.h>


namespace josh {


struct PointShadowMaps {
    using PointMapsTarget = RenderTarget<UniqueAttachment<Renderable::CubemapArray>>;

    PointMapsTarget point_shadow_maps_tgt{
        { 1024, 1024 }, 0, // WARN: TODO: Hold on, this is not legal
        { InternalFormat::DepthComponent32F }
    };
    // TODO: Life would be easier if this was per-light property.
    glm::vec2 z_near_far{ 0.05f, 150.f };
};


} // namespace josh


namespace josh::stages::primary {


class PointShadowMapping {
public:
    PointShadowMapping() = default;

    void operator()(RenderEnginePrimaryInterface& engine);

    auto share_output_view() const noexcept
        -> SharedStorageView<PointShadowMaps>
    {
        return output_.share_view();
    }

    auto view_output() const noexcept
        -> const PointShadowMaps&
    {
        return *output_;
    }

    void resize_maps(const Size2I& new_resolution);

    glm::vec2&       z_near_far()       noexcept { return output_->z_near_far; }
    const glm::vec2& z_near_far() const noexcept { return output_->z_near_far; }


private:
    SharedStorage<PointShadowMaps> output_;

    UniqueProgram sp_with_alpha{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_cubemap.vert"))
            .load_geom(VPath("src/shaders/depth_cubemap_array.geom"))
            .load_frag(VPath("src/shaders/depth_cubemap.frag"))
            .define("ENABLE_ALPHA_TESTING")
            .get()
    };

    UniqueProgram sp_no_alpha{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/depth_cubemap.vert"))
            .load_geom(VPath("src/shaders/depth_cubemap_array.geom"))
            .load_frag(VPath("src/shaders/depth_cubemap.frag"))
            .get()
    };


    void map_point_shadows(RenderEnginePrimaryInterface& engine);

    void resize_cubemap_array_storage_if_needed(
        const entt::registry& registry);

    void draw_all_world_geometry_with_alpha_test(
        BindToken<Binding::Program>         bound_program,
        BindToken<Binding::DrawFramebuffer> bound_fbo,
        const entt::registry&               registry);

    void draw_all_world_geometry_no_alpha_test(
        BindToken<Binding::Program>         bound_program,
        BindToken<Binding::DrawFramebuffer> bound_fbo,
        const entt::registry&               registry);

};




inline void PointShadowMapping::resize_maps(const Size2I& new_resolution) {
    output_->point_shadow_maps_tgt.resize(new_resolution);
}


} // namespace josh::stages::primary
