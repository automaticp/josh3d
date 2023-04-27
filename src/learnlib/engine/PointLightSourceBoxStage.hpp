#pragma once
#include "GLObjects.hpp"
#include "LightCasters.hpp"
#include "MaterialLightSource.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Transform.hpp"
#include "AssimpModelLoader.hpp"
#include "ULocation.hpp"
#include <entt/entt.hpp>
#include <imgui.h>



namespace learn {


class PointLightSourceBoxStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/non_instanced.vert")
            .load_frag("src/shaders/light_source.frag")
            .get()
    };


    Mesh box_{
        AssimpMeshDataLoader<>()
            .load("data/models/container/container.obj")
            .get()[0]
    };

    struct Locations {
        ULocation projection;
        ULocation view;
        ULocation model;
        ULocation normal_model;
        MaterialLightSource::Locations mat_light_source{};
    };

    Locations locs_{
        .projection   = sp_.location_of("projection"),
        .view         = sp_.location_of("view"),
        .model        = sp_.location_of("model"),
        .normal_model = sp_.location_of("normal_model"),
        .mat_light_source = MaterialLightSource::query_locations(sp_)
    };

public:
    float light_box_scale{ 0.2f };

public:
    PointLightSourceBoxStage() = default;

    void operator()(const RenderEngine::PrimaryInterface& engine, const entt::registry& registry) {
        using namespace gl;

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

            ashp.uniform(locs_.projection,
                engine.camera().perspective_projection_mat(
                    engine.window_size().aspect_ratio()
                )
            );
            ashp.uniform(locs_.view, engine.camera().view_mat());

            engine.draw([&, this] {

                auto view = registry.view<light::Point>();
                for (auto [entity, plight] : view.each()) {

                    const auto t = Transform()
                        .translate(plight.position)
                        .scale(glm::vec3{ light_box_scale });

                    ashp.uniform(locs_.model, t.mtransform().model());

                    MaterialLightSource mat{ plight.color };
                    mat.apply(ashp, locs_.mat_light_source);

                    box_.draw();
                }

            });
        });

    }

};





class PointLightSourceBoxStageImGuiHook {
private:
    PointLightSourceBoxStage& stage_;

public:
    PointLightSourceBoxStageImGuiHook(PointLightSourceBoxStage& stage)
        : stage_{ stage }
    {}

    void operator()() {
        ImGui::SliderFloat(
            "Light Box Scale", &stage_.light_box_scale,
            0.001f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );
    }

};

} // namespace learn
