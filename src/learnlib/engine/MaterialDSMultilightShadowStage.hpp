#pragma once
#include "GLObjects.hpp"
#include "GlobalsUtil.hpp"
#include "Layout.hpp"
#include "LightCasters.hpp"
#include "MaterialDS.hpp"
#include "Model.hpp"
#include "RenderEngine.hpp"
#include "RenderTargetColorCubemapArray.hpp"
#include "RenderTargetDepthCubemap.hpp"
#include "RenderTargetDepthCubemapArray.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include "ShaderBuilder.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include "ULocation.hpp"
#include <array>
#include <range/v3/all.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <imgui.h>




namespace learn {


// Tag component that enables shadow rendering for point lights.
struct ShadowComponent {};


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
        ULocation point_light_shadow_maps;
        MaterialDSLocations mat_ds{};
        AmbientLightLocations ambient_light{};
        PointLightLocations point_light{};
    };

    Locations locs_{ Locations{
        .projection   = sp_.location_of("projection"),
        .view         = sp_.location_of("view"),
        .model        = sp_.location_of("model"),
        .normal_model = sp_.location_of("normal_model"),
        .cam_pos      = sp_.location_of("cam_pos"),
        .point_light_z_far =
            sp_.location_of("point_light_z_far"),
        .point_shadow_bias_bounds =
            sp_.location_of("point_shadow_bias_bounds"),
        .point_light_shadow_maps =
            sp_.location_of("point_light_shadow_maps"),
        .mat_ds = MaterialDS::query_locations(sp_),
        .ambient_light = {
            .color = sp_.location_of("ambient_light.color")
        },
    } };

    ShaderProgram sp_plight_depth_{
        ShaderBuilder()
            .load_vert("src/shaders/depth_cubemap.vert")
            .load_shader("src/shaders/depth_cubemap_array.geom", gl::GL_GEOMETRY_SHADER)
            .load_frag("src/shaders/depth_cubemap.frag")
            .get()
    };


    struct LocationsPLight {
        ULocation projection;
        std::array<ULocation, 6> views;
        ULocation cubemap_id;
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
        .cubemap_id = sp_plight_depth_.location_of("cubemap_id"),
        .model      = sp_plight_depth_.location_of("model"),
        .light_pos  = sp_plight_depth_.location_of("light_pos"),
        .z_far      = sp_plight_depth_.location_of("z_far")
    } };


    SSBOWithIntermediateBuffer<light::Point> plights_with_shadows_ssbo_{ 1 };
    SSBOWithIntermediateBuffer<light::Point> plights_no_shadows_ssbo_{ 2 };

    using views_mat_array_t = std::array<glm::mat4, 6>;

public:
    RenderTargetDepthCubemapArray plight_shadow_maps{ 2048, 2048, 0 };
    glm::vec2 plight_z_near_far{ 0.05f, 150.f };
    glm::vec2 point_shadow_bias_bounds{ 0.0001f, 0.03f };


public:
    MaterialDSMultilightShadowStage() = default;

    void operator()(const RenderEngine& engine, entt::registry& registry) {

        using namespace gl;

        // Update SSBOs for point lights.

        auto plights_with_shadow_view = registry.view<const light::Point, const ShadowComponent>();

        const bool num_plights_with_shadows_changed =
            plights_with_shadows_ssbo_.update(
                plights_with_shadow_view | ranges::views::transform([&](entt::entity e) {
                    return plights_with_shadow_view.get<const light::Point>(e);
                })
            );

        if (num_plights_with_shadows_changed) {
            plight_shadow_maps.reset_size(
                plight_shadow_maps.width(), plight_shadow_maps.height(),
                static_cast<GLint>(plights_with_shadows_ssbo_.size())
            );
        }


        auto plights_no_shadow_view = registry.view<const light::Point>(entt::exclude<ShadowComponent>);

        plights_no_shadows_ssbo_.update(
            plights_no_shadow_view | ranges::views::transform([&](entt::entity e) {
                return plights_with_shadow_view.get<const light::Point>(e);
            })
        );


        // Draw the depth cubemaps for Point lights with the ShadowComponent.

        sp_plight_depth_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

            glViewport(0, 0, plight_shadow_maps.width(), plight_shadow_maps.height());

            plight_shadow_maps.framebuffer().bind().and_then_with_self([&, this](BoundFramebuffer& fbo) {


                for (GLint layer{ 0 }; auto [_, plight] : plights_with_shadow_view.each()) {

                    if (layer == 0) /* first time */ {
                        glClear(GL_DEPTH_BUFFER_BIT);
                    }

                    draw_scene_depth_onto_cubemap(ashp, registry, plight.position, layer);

                    // Layer as an index of a cubemap in the array, not as a 'layer-face'
                    ++layer;
                }


            })
            .unbind();
        });


        auto [w, h] = engine.window_size();
        glViewport(0, 0, w, h);

        // Then draw the scene itself.

        sp_.use().and_then_with_self([&, this](ActiveShaderProgram& ashp) {

            ashp.uniform(locs_.projection,
                engine.camera().perspective_projection_mat(
                    engine.window_size().aspect_ratio()
                )
            );
            ashp.uniform(locs_.view, engine.camera().view_mat())
                .uniform(locs_.cam_pos, engine.camera().get_pos());

            // Ambient light.
            for (auto [_, ambi] : registry.view<const light::Ambient>().each()) {
                ashp.uniform(locs_.ambient_light.color, ambi.color);
            }

            // Point light properties are sent through SSBOs.
            // Send the depth cubemap array for point light shadow calculation.
            ashp.uniform(locs_.point_light_shadow_maps, 2);
            plight_shadow_maps.depth_taget().bind_to_unit(GL_TEXTURE2);

            // Extra settings for point light shadows.
            ashp.uniform(locs_.point_light_z_far, plight_z_near_far.y)
                .uniform(locs_.point_shadow_bias_bounds, point_shadow_bias_bounds);

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

private:
    void draw_scene_depth_onto_cubemap(ActiveShaderProgram& ashp,
        entt::registry& registry, const glm::vec3& position, gl::GLint cubemap_id)
    {
        glm::mat4 projection = glm::perspective(
            glm::radians(90.f),
            static_cast<float>(plight_shadow_maps.width()) /
                static_cast<float>(plight_shadow_maps.height()),
            plight_z_near_far.x, plight_z_near_far.y
        );

        ashp.uniform(locs_plight_.projection, projection);

        const auto& basis = globals::basis;
        views_mat_array_t views{
            glm::lookAt(position, position + basis.x(), -basis.y()),
            glm::lookAt(position, position - basis.x(), -basis.y()),
            glm::lookAt(position, position + basis.y(), basis.z()),
            glm::lookAt(position, position - basis.y(), -basis.z()),
            glm::lookAt(position, position + basis.z(), -basis.y()),
            glm::lookAt(position, position - basis.z(), -basis.y()),
        };


        for (size_t i{ 0 }; i < views.size(); ++i) {
            ashp.uniform(locs_plight_.views[i], views[i]);
        }
        ashp.uniform(locs_plight_.cubemap_id, cubemap_id);

        ashp.uniform(locs_plight_.z_far, plight_z_near_far.y);

        for (auto [_, transform, model]
            : registry.view<const Transform, const Shared<Model>>().each())
        {
            ashp.uniform(locs_plight_.model, transform.mtransform().model());
            for (auto& drawable : model->drawable_meshes()) {
                drawable.mesh().draw();
            }
        }

    }


};












class MaterialDSMultilightShadowStageImGuiHook {
private:
    MaterialDSMultilightShadowStage& stage_;
    int shadow_res_;

public:
    MaterialDSMultilightShadowStageImGuiHook(MaterialDSMultilightShadowStage& stage)
        : stage_{ stage }
        , shadow_res_{
            stage_.plight_shadow_maps.width()
        }
    {}


    void operator()() {
        auto& s = stage_;

        if (ImGui::Button("Apply")) {
            s.plight_shadow_maps.reset_size(shadow_res_, shadow_res_, s.plight_shadow_maps.depth());
        }
        ImGui::SameLine();
        ImGui::SliderInt(
            "Point Shadow Resolution", &shadow_res_,
            128, 8192, "%d", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat2(
            "Z Near/Far", glm::value_ptr(s.plight_z_near_far),
            0.01f, 500.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat2(
            "Shadow Bias", glm::value_ptr(s.point_shadow_bias_bounds),
            0.00001f, 0.1f, "%.5f", ImGuiSliderFlags_Logarithmic
        );

    }

};






} // namespace learn
