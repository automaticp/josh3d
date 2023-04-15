#pragma once
#include "GLObjects.hpp"
#include "LightCasters.hpp"
#include "MaterialLightSource.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Transform.hpp"
#include "AssimpModelLoader.hpp"
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


    Mesh box_{
        AssimpMeshDataLoader<>()
            .load("data/models/container/container.obj")
            .get()[0]
    };

    struct Locations {
        gl::GLint projection{ -1 };
        gl::GLint view{ -1 };
        gl::GLint model{ -1 };
        gl::GLint normal_model{ -1 };
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
    PointLightSourceBoxStage() = default;

    void operator()(const RenderEngine& engine, entt::registry& registry) {
        using namespace gl;

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

            ashp.uniform(locs_.projection,
                engine.camera().perspective_projection_mat(
                    engine.window_size().aspect_ratio()
                )
            );
            ashp.uniform(locs_.view, engine.camera().view_mat());

            auto view = registry.view<light::Point>();
            for (auto [entity, plight] : view.each()) {

                const auto t = Transform()
                    .translate(plight.position)
                    .scale(glm::vec3{ 0.2f });

                ashp.uniform(locs_.model, t.mtransform().model());

                MaterialLightSource mat{ plight.color };
                mat.apply(ashp, locs_.mat_light_source);

                box_.draw();
            }

        });

    }

};


} // namespace learn
