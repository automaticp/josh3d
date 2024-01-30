#pragma once
#include "GLObjects.hpp"
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "Mesh.hpp"
#include "DefaultResources.hpp"
#include <entt/entt.hpp>




namespace josh::stages::primary {


class PointLightBox {
private:
    UniqueShaderProgram sp_{
        ShaderBuilder()
            .load_vert(VPath("src/shaders/basic_mesh.vert"))
            .load_frag(VPath("src/shaders/light_source.frag"))
            .get()
    };

    Mesh box_{ globals::box_primitive_data() };

public:
    float light_box_scale{ 0.1f };
    bool  display{ true };

public:
    PointLightBox() = default;

    void operator()(const RenderEnginePrimaryInterface& engine, const entt::registry& registry) {
        using namespace gl;

        if (!display) { return; }

        sp_.use().and_then([&, this](ActiveShaderProgram<GLMutable>& ashp) {

            ashp.uniform("projection", engine.camera().projection_mat())
                .uniform("view",       engine.camera().view_mat());

            engine.draw([&, this] {

                auto view = registry.view<light::Point>();
                for (auto [entity, plight] : view.each()) {

                    const auto t = Transform()
                        .translate(plight.position)
                        .scale(glm::vec3{ light_box_scale });

                    ashp.uniform("model", t.mtransform().model())
                        .uniform("light_color", plight.color);

                    box_.draw();
                }

            });
        });

    }

};


} // namespace josh::stages::primary
