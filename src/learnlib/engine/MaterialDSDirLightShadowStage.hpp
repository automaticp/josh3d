#pragma once
#include "GLObjects.hpp"
#include "MaterialDS.hpp"
#include "Model.hpp"
#include "RenderEngine.hpp"
#include "RenderTargetDepth.hpp"
#include "ShaderBuilder.hpp"
#include "LightCasters.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include "ULocation.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>



namespace learn {


// These names, man...
class MaterialDSDirLightShadowStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/in_directional_shadow.vert")
            .load_frag("src/shaders/mat_ds_light_ad_shadow.frag")
            .get()
    };

    ShaderProgram sp_depth_{
        ShaderBuilder()
            .load_vert("src/shaders/depth_map.vert")
            .load_frag("src/shaders/depth_map.frag")
            .get()
    };


    struct AmbientLightLocations {
        ULocation color;
    };

    struct DirectionalLightLocations {
        ULocation color;
        ULocation direction;
    };

    struct Locations {
        ULocation projection;
        ULocation view;
        ULocation model;
        ULocation normal_model;
        ULocation dir_light_pv;
        ULocation cam_pos;
        MaterialDSLocations mat_ds{};
        AmbientLightLocations ambient_light{};
        DirectionalLightLocations dir_light{};
        ULocation shadow_map;
        ULocation shadow_bias_bounds;
    };

    Locations locs_{
        .projection     = sp_.location_of("projection"),
        .view           = sp_.location_of("view"),
        .model          = sp_.location_of("model"),
        .normal_model   = sp_.location_of("normal_model"),
        .dir_light_pv   = sp_.location_of("dir_light_pv"),
        .cam_pos        = sp_.location_of("cam_pos"),
        .mat_ds         = MaterialDS::query_locations(sp_),
        .ambient_light  = {
            .color = sp_.location_of("ambient_light.color")
        },
        .dir_light      = {
            .color = sp_.location_of("dir_light.color"),
            .direction = sp_.location_of("dir_light.direction")
        },
        .shadow_map     = sp_.location_of("shadow_map"),
        .shadow_bias_bounds = sp_.location_of("shadow_bias_bounds")
    };


    struct LocationsDepth {
        ULocation projection;
        ULocation view;
        ULocation model;
    };

    LocationsDepth locs_depth_{
        .projection = sp_depth_.location_of("projection"),
        .view       = sp_depth_.location_of("view"),
        .model      = sp_depth_.location_of("model")
    };


public:
    RenderTargetDepth depth_target{ 4096, 4096 };

    glm::vec2 shadow_bias_bounds{ 0.0001f, 0.0015f };
    float light_projection_scale{ 50.f };
    glm::vec2 light_z_near_far{ 15.f, 150.f };
    float camera_offset{ 100.f };


    void operator()(const RenderEngine& engine, entt::registry& registry) {
        using namespace gl;

        for (
            auto [ent, dir_light]
                : registry
                    .view<light::Directional>()
                    .each()
        ) {

            glm::mat4 light_projection = glm::ortho(
                -light_projection_scale, light_projection_scale,
                -light_projection_scale, light_projection_scale,
                light_z_near_far.x, light_z_near_far.y
            );

            glm::mat4 light_view = glm::lookAt(
                engine.camera().get_pos() - camera_offset * glm::normalize(dir_light.direction),
                engine.camera().get_pos(),
                // -directional_.direction,
                // glm::vec3(0.f),
                globals::basis.y()
            );

            glViewport(0, 0, depth_target.width(), depth_target.height());

            depth_target.framebuffer().bind()
                .and_then([&] {
                    glClear(GL_DEPTH_BUFFER_BIT);
                    draw_scene_depth(registry, light_projection, light_view);
                })
                .unbind();

            auto [w, h] = engine.window_size();
            glViewport(0, 0, w, h);

            draw_scene_objects(engine, registry, light_projection * light_view);
        }
    }


private:
    void draw_scene_objects(const RenderEngine& engine,
        entt::registry& registry, const glm::mat4& light_mvp)
    {
        using namespace gl;

        glm::mat4 projection =
            engine.camera().perspective_projection_mat(
                engine.window_size().aspect_ratio()
            );

        glm::mat4 view = engine.camera().view_mat();

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

            ashp.uniform(locs_.projection, projection)
                .uniform(locs_.view, view);

            ashp.uniform(locs_.dir_light_pv, light_mvp)
                .uniform(locs_.shadow_bias_bounds, shadow_bias_bounds)
                .uniform(locs_.shadow_map, 2);
            depth_target.depth_target().bind_to_unit(GL_TEXTURE2);

            for (auto [_, ambi] : registry.view<const light::Ambient>().each()) {
                ashp.uniform(locs_.ambient_light.color, ambi.color);
            }

            for (auto [_, dir] : registry.view<const light::Directional>().each()) {
                ashp.uniform(locs_.dir_light.color, dir.color)
                    .uniform(locs_.dir_light.direction, dir.direction);
            }

            auto view = registry.view<const Transform, const Shared<Model>>();
            for (auto [_, transform, model] : view.each()) {

                auto model_transform = transform.mtransform();
                ashp.uniform(locs_.model, model_transform.model())
                    .uniform(locs_.normal_model, model_transform.normal_model());

                model->draw(ashp, locs_.mat_ds);
            }


        });

    }


    void draw_scene_depth(entt::registry& registry,
        const glm::mat4& projection, const glm::mat4& view)
    {
        sp_depth_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

            ashp.uniform(locs_depth_.projection, projection)
                .uniform(locs_depth_.view, view);

            auto view = registry.view<const Transform, const Shared<Model>>();

            for (auto [_, transform, model] : view.each()) {

                ashp.uniform(locs_depth_.model, transform.mtransform().model());

                for (auto& drawable : model->drawable_meshes()) {
                    drawable.mesh().draw();
                }
            }
        });
    }
};









} // namespace learn
