#pragma once
#include "GLObjects.hpp"
#include "LightCasters.hpp"
#include "MaterialDS.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include "Model.hpp"
#include "ULocation.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <range/v3/all.hpp>
#include <glbinding/gl/gl.h>

namespace learn {


class MaterialDSMultilightStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/non_instanced.vert")
            .load_frag("src/shaders/mat_ds_light_adpn.frag")
            .get()
    };

    SSBOWithIntermediateBuffer<light::Point> plights_ssbo_{
        1, gl::GL_DYNAMIC_DRAW
    };

    struct AmbientLightLocations {
        ULocation color;
    };

    struct DirectionalLightLocations {
        ULocation color;
        ULocation direction;
    };

    // Eww
    struct Locations {
        ULocation projection;
        ULocation view;
        ULocation model;
        ULocation normal_model;
        ULocation cam_pos;
        MaterialDSLocations mat_ds{};
        AmbientLightLocations ambient_light{};
        DirectionalLightLocations dir_light{};
    };

    Locations locs_{
        .projection     = sp_.location_of("projection"),
        .view           = sp_.location_of("view"),
        .model          = sp_.location_of("model"),
        .normal_model   = sp_.location_of("normal_model"),
        .cam_pos        = sp_.location_of("cam_pos"),
        .mat_ds         = MaterialDS::query_locations(sp_),
        .ambient_light  = {
            .color = sp_.location_of("ambient_light.color")
        },
        .dir_light = {
            .color = sp_.location_of("dir_light.color"),
            .direction = sp_.location_of("dir_light.direction")
        }
    };

public:
    MaterialDSMultilightStage() = default;

    void operator()(const RenderEngine::PrimaryInterface& engine, const entt::registry& registry) {
        using namespace gl;

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

            auto plight_view = registry.view<const light::Point>();

            plights_ssbo_.update(
                plight_view | ranges::views::transform([&](entt::entity e) {
                    return plight_view.get<const light::Point>(e);
                })
            );

            ashp.uniform(locs_.projection,
                engine.camera().perspective_projection_mat(
                    engine.window_size().aspect_ratio()
                )
            );
            ashp.uniform(locs_.view, engine.camera().view_mat());

            ashp.uniform(locs_.cam_pos, engine.camera().get_pos());


            // Ambient light
            // FIXME: Must be unique. What now?
            for (auto [e, ambi] : registry.view<const light::Ambient>().each()) {
                ashp.uniform(locs_.ambient_light.color, ambi.color);
            }

            // Directional light
            for (auto [e, dir] : registry.view<const light::Directional>().each()) {
                ashp.uniform(locs_.dir_light.color, dir.color)
                    .uniform(locs_.dir_light.direction, dir.direction);
            }


            engine.draw([&, this] {

                auto view = registry.view<const Transform, const Shared<Model>>();

                for (auto [entity, transform, model] : view.each()) {

                    auto model_matrix = transform.mtransform();
                    ashp.uniform(locs_.model, model_matrix.model())
                        .uniform(locs_.normal_model, model_matrix.normal_model());

                    model->draw(ashp, locs_.mat_ds);

                }

            });
        });

    }
};


} // namespace learn
