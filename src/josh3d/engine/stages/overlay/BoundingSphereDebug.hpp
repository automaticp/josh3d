#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "components/BoundingSphere.hpp"
#include "Mesh.hpp"
#include "DefaultResources.hpp"
#include "ECSHelpers.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>


namespace josh::stages::overlay {


class BoundingSphereDebug {
private:
    UniqueShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/basic_mesh.vert"))
            .load_frag(VPath("src/shaders/light_source.frag"))
            .get()
    };

    Mesh sphere_{ globals::sphere_primitive_data() };

public:
    bool display      { false };
    bool selected_only{ true  };

    glm::vec3 line_color{ 1.f, 1.f, 1.f };
    float     line_width{ 2.f };

    void operator()(
        const RenderEngineOverlayInterface& engine,
        const entt::registry& registry)
    {

        if (!display) { return; }
        if (selected_only && registry.view<tags::Selected>().empty()) { return; }

        using namespace gl;

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glEnable(GL_DEPTH_TEST);
        glLineWidth(line_width);

        sp_ .use()
            .uniform("projection",  engine.camera().projection_mat())
            .uniform("view",        engine.camera().view_mat())
            .uniform("light_color", line_color)
            .and_then([&, this](ActiveShaderProgram<>& ashp) {
                engine.draw([&, this] {

                    auto per_mesh_draw_func = [&] (
                        entt::entity entity,
                        const Transform& transform,
                        const components::BoundingSphere& sphere)
                    {
                        const auto full_mtransform =
                            get_full_mesh_mtransform({ registry, entity }, transform.mtransform());

                        const glm::vec3 sphere_center = full_mtransform.decompose_position();
                        const glm::vec3 mesh_scaling  = full_mtransform.decompose_local_scale();

                        const glm::vec3 sphere_scale = glm::vec3{ sphere.scaled_radius(mesh_scaling) };

                        const auto sphere_transf = Transform().translate(sphere_center).scale(sphere_scale);

                        ashp.uniform("model", sphere_transf.mtransform().model());
                        sphere_.draw();

                    };

                    if (selected_only) {
                        registry.view<Transform, components::BoundingSphere, tags::Selected>()
                            .each(per_mesh_draw_func);
                    } else {
                        registry.view<Transform, components::BoundingSphere>()
                            .each(per_mesh_draw_func);
                    }

                });
            });

        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    }


};


} // namespace josh::stages::overlay
