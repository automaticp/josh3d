#pragma once
#include "GLObjects.hpp"
#include "LightCasters.hpp"
#include "MaterialDS.hpp"
#include "RenderEngine.hpp"
#include "ShaderBuilder.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include "Model.hpp"
#include <entt/entt.hpp>
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

    SSBO point_lights_ssbo_;
    std::vector<light::Point> temp_storage_;

    struct AmbientLightLocations {
        gl::GLint color{ -1 };
    };

    struct DirectionalLightLocations {
        gl::GLint color{ -1 };
        gl::GLint direction{ -1 };
    };

    // Eww
    struct Locations {
        gl::GLint projection{ -1 };
        gl::GLint view{ -1 };
        gl::GLint model{ -1 };
        gl::GLint normal_model{ -1 };
        gl::GLint cam_pos{ -1 };
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

    void operator()(const RenderEngine& engine, entt::registry& registry) {
        using namespace gl;

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

            // Update SSBO for point lights.
            //
            // FIXME: Figure out if entt can provide contigious storage by default.
            auto point_light_view = registry.view<light::Point>();

            point_lights_ssbo_.bind_to(1).and_then_with_self([&, this](BoundSSBO& ssbo) {
                const bool needs_resize =
                    point_light_view.size() != temp_storage_.size();

                if (needs_resize) {
                    temp_storage_.reserve(point_light_view.size());
                }

                // Copy into the temp storage
                temp_storage_.clear();
                for (auto [entity, light] : point_light_view.each()) {
                    temp_storage_.emplace_back(light);
                }

                if (needs_resize) {
                    ssbo.attach_data(
                        temp_storage_.size(), temp_storage_.data(),
                        GL_STATIC_DRAW
                    );
                } else {
                    ssbo.sub_data(temp_storage_.size(), 0, temp_storage_.data());
                }
            });

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

            auto view = registry.view<const Transform, const Shared<Model>>();

            for (auto [entity, transform, model] : view.each()) {

                auto model_matrix = transform.mtransform();
                ashp.uniform(locs_.model, model_matrix.model())
                    .uniform(locs_.normal_model, model_matrix.normal_model());

                model->draw(ashp, locs_.mat_ds);

            }
        });

    }
};


} // namespace learn
