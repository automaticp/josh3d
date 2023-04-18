#pragma once
#include "GLObjects.hpp"
#include "GlobalsUtil.hpp"
#include "LightCasters.hpp"
#include "MaterialDS.hpp"
#include "Model.hpp"
#include "RenderEngine.hpp"
#include "RenderTargetDepthCubemap.hpp"
#include "ShaderBuilder.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include "ULocation.hpp"
#include <array>
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>





namespace learn {


class MaterialDSMultilightShadowStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/non_instanced.vert")
            .load_frag("src/shaders/mat_ds_light_apn_shadow.frag")
            .get()
    };

    struct AmbientLightLocations {
        ULocation color;
    };

    struct AttenuationLocations {
        ULocation constant;
        ULocation linear;
        ULocation quadratic;
    };

    struct PointLightLocations {
        ULocation color;
        ULocation position;
        AttenuationLocations attenuation;
    };

    struct Locations {
        ULocation projection;
        ULocation view;
        ULocation model;
        ULocation normal_model;
        ULocation cam_pos;
        ULocation point_light_z_far;
        ULocation point_shadow_bias_bounds;
        ULocation point_shadow_map;
        MaterialDSLocations mat_ds{};
        AmbientLightLocations ambient_light{};
        PointLightLocations point_light{};
    };

    Locations locs_{ Locations{
        .projection = sp_.location_of("projection"),
        .view = sp_.location_of("view"),
        .model = sp_.location_of("model"),
        .normal_model = sp_.location_of("normal_model"),
        .cam_pos = sp_.location_of("cam_pos"),
        .point_light_z_far =
            sp_.location_of("point_light_z_far"),
        .point_shadow_bias_bounds =
            sp_.location_of("point_shadow_bias_bounds"),
        .point_shadow_map =
            sp_.location_of("point_shadow_map"),
        .mat_ds = MaterialDS::query_locations(sp_),
        .ambient_light = {
            .color = sp_.location_of("ambient_light.color")
        },
        .point_light = {
            .color = sp_.location_of("point_light.color"),
            .position = sp_.location_of("point_light.position"),
            .attenuation = {
                .constant =
                    sp_.location_of("point_light.attenuation.constant"),
                .linear =
                    sp_.location_of("point_light.attenuation.linear"),
                .quadratic =
                    sp_.location_of("point_light.attenuation.quadratic")
            }
        }
    } };

    ShaderProgram sp_plight_depth_{
        ShaderBuilder()
            .load_vert("src/shaders/depth_cubemap.vert")
            .load_shader("src/shaders/depth_cubemap.geom", gl::GL_GEOMETRY_SHADER)
            .load_frag("src/shaders/depth_cubemap.frag")
            .get()
    };


    struct LocationsPLight {
        ULocation projection;
        std::array<ULocation, 6> views;
        ULocation model;
        ULocation light_pos;
        ULocation z_far;
    };

    LocationsPLight locs_plight_{ LocationsPLight{
        .projection = sp_plight_depth_.location_of("projection"),
        .views = {
            sp_plight_depth_.location_of("views[0]"),
            sp_plight_depth_.location_of("views[1]"),
            sp_plight_depth_.location_of("views[2]"),
            sp_plight_depth_.location_of("views[3]"),
            sp_plight_depth_.location_of("views[4]"),
            sp_plight_depth_.location_of("views[5]"),
        },
        .model = sp_plight_depth_.location_of("model"),
        .light_pos = sp_plight_depth_.location_of("light_pos"),
        .z_far = sp_plight_depth_.location_of("z_far")
    } };


public:
    glm::vec2 plight_z_near_far{ 1.f, 25.f };
    glm::vec2 point_shadow_bias_bounds{ 0.0001f, 0.0015f };


public:
    MaterialDSMultilightShadowStage() = default;

    void operator()(const RenderEngine& engine, entt::registry& registry) {

        using namespace gl;

        // TODO: This currently draws to all of the cubemaps
        // but will only render shadows for one of them.

        // Render all depth cubemaps.
        for (auto [_, plight, depth_target]
            : registry.view<const light::Point, RenderTargetDepthCubemap>().each())
        {

            sp_plight_depth_.use().and_then_with_self(
                [&, &depth_target=depth_target, &plight=plight, this](ActiveShaderProgram& ashp) {

                    glViewport(0, 0, depth_target.width(), depth_target.height());

                    depth_target.framebuffer().bind()
                        .and_then([&, this] {
                            glClear(GL_DEPTH_BUFFER_BIT);
                            // Draw scene depth.

                            glm::mat4 projection = glm::perspective(
                                glm::radians(90.f),
                                static_cast<float>(depth_target.width()) /
                                    static_cast<float>(depth_target.height()),
                                plight_z_near_far.x, plight_z_near_far.y
                            );

                            const auto& basis = globals::basis;
                            std::array<glm::mat4, 6> views{
                                glm::lookAt(plight.position, plight.position + basis.x(), -basis.y()),
                                glm::lookAt(plight.position, plight.position - basis.x(), -basis.y()),
                                // FIXME: What happens when the global Up is the same?
                                glm::lookAt(plight.position, plight.position + basis.y(), basis.z()),
                                glm::lookAt(plight.position, plight.position - basis.y(), -basis.z()),
                                glm::lookAt(plight.position, plight.position + basis.z(), -basis.y()),
                                glm::lookAt(plight.position, plight.position - basis.z(), -basis.y()),
                            };

                            ashp.uniform(locs_plight_.projection, projection);
                            for (size_t i{ 0 }; i < views.size(); ++i) {
                                ashp.uniform(locs_plight_.views[i], views[i]);
                            }

                            ashp.uniform(locs_plight_.light_pos, plight.position)
                                .uniform(locs_plight_.z_far, plight_z_near_far.y);

                            for (auto [_, transform, model]
                                : registry.view<const Transform, const Shared<Model>>().each())
                            {
                                ashp.uniform(locs_plight_.model, transform.mtransform().model());
                                for (auto& drawable : model->drawable_meshes()) {
                                    drawable.mesh().draw();
                                }
                            }

                        })
                        .unbind();

                }
            );
        }

        auto [w, h] = engine.window_size();
        glViewport(0, 0, w, h);


        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

            ashp.uniform(locs_.projection,
                engine.camera().perspective_projection_mat(
                    engine.window_size().aspect_ratio()
                )
            );
            ashp.uniform(locs_.view, engine.camera().view_mat());

            ashp.uniform(locs_.cam_pos, engine.camera().get_pos());

            // Ambient light.
            for (auto [_, ambi] : registry.view<const light::Ambient>().each()) {
                ashp.uniform(locs_.ambient_light.color, ambi.color);
            }

            // Point light. Must be unique for now. Once not unique, won't need this loop.
            for (auto [_, plight] : registry.view<const light::Point>().each()) {
                ashp.uniform(locs_.point_light.color, plight.color)
                    .uniform(locs_.point_light.position, plight.position)
                    .uniform(locs_.point_light.attenuation.constant, plight.attenuation.constant)
                    .uniform(locs_.point_light.attenuation.linear, plight.attenuation.linear)
                    .uniform(locs_.point_light.attenuation.quadratic, plight.attenuation.quadratic);
            }
            // Extra settings for point shadow.
            ashp.uniform(locs_.point_light_z_far, plight_z_near_far.y)
                .uniform(locs_.point_shadow_bias_bounds, point_shadow_bias_bounds);

            for (auto [_, plight, shadow_map]
                : registry.view<const light::Point, RenderTargetDepthCubemap>().each())
            {
                shadow_map.depth_taget().bind_to_unit(GL_TEXTURE2);
                ashp.uniform(locs_.point_shadow_map, 2);
            }

            for (auto [_, transform, model]
                : registry.view<const Transform, const Shared<Model>>().each())
            {
                auto model_transform = transform.mtransform();
                ashp.uniform(locs_.model, model_transform.model())
                    .uniform(locs_.normal_model, model_transform.normal_model());

                model->draw(ashp, locs_.mat_ds);
            }

        });

    }


};


} // namespace learn
