#pragma once
#include "GLObjects.hpp"
#include "UniformTraits.hpp" // IWYU pragma: keep (traits)
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "components/BoundingSphere.hpp"
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
            .load_vert(VPath("src/shaders/basic_mesh.vert"))
            .load_frag(VPath("src/shaders/light_source.frag"))
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


    sp_->uniform("projection",  engine.camera().projection_mat());
    sp_->uniform("view",        engine.camera().view_mat());
    sp_->uniform("light_color", line_color);

    auto bound_program = sp_->use();

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

        if (selected_only) {
            registry.view<Transform, components::BoundingSphere, tags::Selected>().each(per_mesh_draw_func);
        } else {
            registry.view<Transform, components::BoundingSphere>().each(per_mesh_draw_func);
        }
    });

    bound_program.unbind();

    glapi::disable(Capability::DepthTesting);
    glapi::set_polygon_rasterization_mode(PolygonRaserization::Fill);

}




} // namespace josh::stages::overlay
