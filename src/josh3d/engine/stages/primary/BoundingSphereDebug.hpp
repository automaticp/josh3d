#pragma once
#include "GLObjects.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "components/ChildMesh.hpp"
#include "components/BoundingSphere.hpp"
#include "Mesh.hpp"
#include "DefaultResources.hpp"
#include <entt/entt.hpp>


namespace josh::stages::primary {


class BoundingSphereDebug {
private:
    UniqueShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/non_instanced.vert"))
            .load_frag(VPath("src/shaders/light_source.frag"))
            .get()
    };

    Mesh sphere_{ globals::sphere_primitive_data() };

public:
    bool display{ false };
    glm::vec3 sphere_color{ 0.f, 1.f, 0.f };

    void operator()(const RenderEnginePrimaryInterface& engine,
        const entt::registry& registry)
    {
        if (!display) { return; }

        using namespace gl;

        auto get_full_transform = [&](entt::entity ent, const Transform& transform) {
            if (auto as_child = registry.try_get<components::ChildMesh>(ent)) {
                return registry.get<Transform>(as_child->parent) * transform;
            } else {
                return transform;
            }
        };

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        sp_ .use()
            .uniform("projection", engine.camera().projection_mat())
            .uniform("view", engine.camera().view_mat())
            .uniform("light_color", sphere_color)
            .and_then([&, this](ActiveShaderProgram<GLMutable>& ashp) {
                engine.draw([&, this] {
                    auto view = registry.view<Transform, components::BoundingSphere>();
                    for (auto [entity, transform, sphere] : view.each()) {

                        auto full_transform = get_full_transform(entity, transform);

                        const glm::vec3 sphere_scale =
                            glm::vec3{ sphere.scaled_radius(full_transform.scaling()) };

                        const auto sphere_transf = Transform()
                            .translate(full_transform.position())
                            .scale(sphere_scale);

                        ashp.uniform("model", sphere_transf.mtransform().model());
                        sphere_.draw();
                    }
                });
            });

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    }


};


} // namespace josh::stages::primary
