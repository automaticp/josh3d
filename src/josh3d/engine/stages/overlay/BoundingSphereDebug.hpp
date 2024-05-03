#pragma once
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "LightCasters.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "components/BoundingSphere.hpp"
#include "components/Transform.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>


namespace josh::stages::overlay {


class BoundingSphereDebug {
public:
    bool display      { false };
    bool selected_only{ true  };

    glm::vec3 line_color{ 1.f, 1.f, 1.f };
    float     line_width{ 2.f };

    void operator()(RenderEngineOverlayInterface& engine);


private:
    UniqueProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/ovl_bounding_volumes.vert"))
            .load_frag(VPath("src/shaders/ovl_bounding_volumes.frag"))
            .get()
    };

};




inline void BoundingSphereDebug::operator()(
    RenderEngineOverlayInterface& engine)
{
    const auto& registry = engine.registry();

    if (!display) { return; }
    if (selected_only && registry.view<tags::Selected>().empty()) { return; }


    glapi::enable(Capability::DepthTesting);
    glapi::set_polygon_rasterization_mode(PolygonRaserization::Line);
    glapi::set_line_width(line_width);

    BindGuard bound_camera_ubo = engine.bind_camera_ubo();
    sp_->uniform("color", line_color);

    BindGuard bound_program = sp_->use();

    engine.draw([&, this](auto bound_fbo) {

        auto per_mesh_draw_func = [&] (
            const entt::entity&               entity [[maybe_unused]],
            const MTransform&                 world_mtf,
            const components::BoundingSphere& sphere)
        {

            const glm::vec3 sphere_center = world_mtf.decompose_position();
            const glm::vec3 mesh_scaling  = world_mtf.decompose_local_scale();
            const glm::vec3 sphere_scale  = glm::vec3{ sphere.scaled_radius(mesh_scaling) };

            const auto sphere_transf = Transform().translate(sphere_center).scale(sphere_scale);

            sp_->uniform("model", sphere_transf.mtransform().model());

            engine.primitives().sphere_mesh().draw(bound_program, bound_fbo);
        };

        auto per_plight_draw_func = [&] (
            const entt::entity&               entity [[maybe_unused]],
            const light::Point&               plight,
            const components::BoundingSphere& sphere)
        {
            const glm::vec3 sphere_scale = glm::vec3{ sphere.radius };
            const auto sphere_transf = Transform().translate(plight.position).scale(sphere_scale);

            sp_->uniform("model", sphere_transf.mtransform().model());

            engine.primitives().sphere_mesh().draw(bound_program, bound_fbo);
        };


        if (selected_only) {
            registry.view<components::MTransform, components::BoundingSphere, tags::Selected>().each(per_mesh_draw_func);
            registry.view<light::Point, components::BoundingSphere, tags::Selected>().each(per_plight_draw_func);
        } else {
            registry.view<components::MTransform, components::BoundingSphere>().each(per_mesh_draw_func);
            registry.view<light::Point, components::BoundingSphere>().each(per_plight_draw_func);
        }
    });

    glapi::disable(Capability::DepthTesting);
    glapi::set_polygon_rasterization_mode(PolygonRaserization::Fill);

}




} // namespace josh::stages::overlay
