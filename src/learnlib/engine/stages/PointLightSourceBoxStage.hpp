#pragma once
#include "GLObjects.hpp"
#include "GlobalsData.hpp"
#include "LightCasters.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Transform.hpp"
#include <entt/entt.hpp>



namespace learn {


class PointLightSourceBoxStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/non_instanced.vert")
            .load_frag("src/shaders/light_source.frag")
            .get()
    };


    Mesh box_{ globals::box_primitive() };

public:
    float light_box_scale{ 0.1f };

public:
    PointLightSourceBoxStage() = default;

    void operator()(const RenderEnginePrimaryInterface& engine, const entt::registry& registry) {
        using namespace gl;

        sp_.use().and_then([&, this](ActiveShaderProgram& ashp) {

            ashp.uniform("projection",
                engine.camera().perspective_projection_mat(
                    engine.window_size().aspect_ratio()
                )
            );
            ashp.uniform("view", engine.camera().view_mat());

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




} // namespace learn
