#pragma once
#include "GLObjects.hpp"
#include "GlobalsUtil.hpp"
#include "LightCasters.hpp"
#include "MaterialDS.hpp"
#include "Model.hpp"
#include "RenderEngine.hpp"
#include "RenderTargetDepth.hpp"
#include "RenderTargetDepthCubemapArray.hpp"
#include "SSBOWithIntermediateBuffer.hpp"
#include "ShaderBuilder.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include "ULocation.hpp"
#include "RenderComponents.hpp"
#include <array>
#include <glbinding/gl/enum.h>
#include <range/v3/all.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <imgui.h>




namespace learn {

/*
Forward rendering stage for meshes with diffuse-specular material.

Supports ambient light, 1 directional light with (optional) shadows,
and "unbounded" number of point lights with (optional) shadows.

You'll run out of frames and memory for large number
of point light shadows (>2) though.
*/
class MaterialDSMultilightShadowStage {
private:
    ShaderProgram sp_{
        ShaderBuilder()
            .load_vert("src/shaders/in_directional_shadow.vert")
            .load_frag("src/shaders/mat_ds_light_adpn_shadow.frag")
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
            .load_geom("src/shaders/depth_cubemap_array.geom")
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

    ShaderProgram sp_dir_depth_{
        ShaderBuilder()
            .load_vert("src/shaders/depth_map.vert")
            .load_frag("src/shaders/depth_map.frag")
            .get()
    };


    SSBOWithIntermediateBuffer<light::Point> plights_with_shadows_ssbo_{
        1, gl::GL_DYNAMIC_DRAW
    };
    SSBOWithIntermediateBuffer<light::Point> plights_no_shadows_ssbo_{
        2, gl::GL_DYNAMIC_DRAW
    };

    using views_mat_array_t = std::array<glm::mat4, 6>;

public:
    RenderTargetDepthCubemapArray plight_shadow_maps{ 1024, 1024, 0 };
    glm::vec2 plight_z_near_far{ 0.05f, 150.f };
    glm::vec2 point_shadow_bias_bounds{ 0.0001f, 0.08f };
    gl::GLint point_light_pcf_samples{ 1 };
    bool point_light_use_fixed_pcf_samples{ true };
    float point_light_pcf_offset{ 0.01f };


    RenderTargetDepth dir_light_shadow_map{ 4096, 4096 };
    glm::vec2 dir_shadow_bias_bounds{ 0.0001f, 0.0015f };
    float     dir_light_projection_scale{ 50.f };
    glm::vec2 dir_light_z_near_far{ 15.f, 150.f };
    float     dir_light_cam_offset{ 100.f };
    gl::GLint dir_light_pcf_samples{ 1 };


    MaterialDSMultilightShadowStage() = default;

    void operator()(const RenderEngine::PrimaryInterface& engine, const entt::registry& registry) {
        using namespace gl;

        prepare_point_lights(engine, registry);

        glm::mat4 dir_light_pv =
            prepare_dir_light(engine, registry);

        engine.draw([&, this] {
            draw_scene(engine, registry, dir_light_pv);
        });
    }

private:
    void prepare_point_lights(const RenderEngine::PrimaryInterface&,
        const entt::registry& registry);


    void draw_scene_depth_cubemap(ActiveShaderProgram& ashp,
        const entt::registry& registry, const glm::vec3& position,
        gl::GLint cubemap_id);


    glm::mat4 prepare_dir_light(const RenderEngine::PrimaryInterface& engine,
        const entt::registry& registry);


    void draw_scene(const RenderEngine::PrimaryInterface& engine,
        const entt::registry& registry, const glm::mat4& dir_light_pv);


};




inline void MaterialDSMultilightShadowStage::prepare_point_lights(
    const RenderEngine::PrimaryInterface&,
    const entt::registry& registry)
{

    using namespace gl;

    // Update SSBOs for point lights.

    auto plights_with_shadow_view = registry.view<const light::Point, const components::ShadowCasting>();

    const size_t old_size = plights_with_shadows_ssbo_.size();

    plights_with_shadows_ssbo_.bind().update(
        plights_with_shadow_view | ranges::views::transform([&](entt::entity e) {
            return plights_with_shadow_view.get<const light::Point>(e);
        })
    );

    const bool num_plights_with_shadows_changed =
        old_size != plights_with_shadows_ssbo_.size();

    if (num_plights_with_shadows_changed) {
        plight_shadow_maps.reset_size(
            plight_shadow_maps.width(), plight_shadow_maps.height(),
            static_cast<GLint>(plights_with_shadows_ssbo_.size())
        );
    }


    auto plights_no_shadow_view = registry.view<const light::Point>(entt::exclude<components::ShadowCasting>);

    plights_no_shadows_ssbo_.bind().update(
        plights_no_shadow_view | ranges::views::transform([&](entt::entity e) {
            return plights_with_shadow_view.get<const light::Point>(e);
        })
    );


    // Draw the depth cubemaps for Point lights with the components::ShadowCasting.

    sp_plight_depth_.use()
        .and_then_with_self([&, this](ActiveShaderProgram& ashp) {

        glViewport(0, 0, plight_shadow_maps.width(), plight_shadow_maps.height());

        plight_shadow_maps.framebuffer().bind()
            .and_then([&, this] {

                for (GLint layer{ 0 }; auto [_, plight]
                    : plights_with_shadow_view.each())
                {
                    if (layer == 0) /* first time */ {
                        glClear(GL_DEPTH_BUFFER_BIT);
                    }

                    draw_scene_depth_cubemap(ashp, registry, plight.position, layer);

                    // Layer as an index of a cubemap in the array, not as a 'layer-face'
                    ++layer;
                }

            })
            .unbind();
        });

}




inline void MaterialDSMultilightShadowStage::draw_scene_depth_cubemap(
    ActiveShaderProgram& ashp, const entt::registry& registry,
    const glm::vec3& position, gl::GLint cubemap_id)
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




inline glm::mat4 MaterialDSMultilightShadowStage::prepare_dir_light(
    const RenderEngine::PrimaryInterface& engine,
    const entt::registry& registry)
{

    using namespace gl;

    glm::mat4 light_pv{};


    for (auto [_, dir_light]
        : registry.view<const light::Directional, components::ShadowCasting>().each())
    {
        glViewport(0, 0, dir_light_shadow_map.width(), dir_light_shadow_map.height());

        glm::mat4 light_projection = glm::ortho(
            -dir_light_projection_scale, dir_light_projection_scale,
            -dir_light_projection_scale, dir_light_projection_scale,
            dir_light_z_near_far.x, dir_light_z_near_far.y
        );

        glm::mat4 light_view = glm::lookAt(
            engine.camera().get_pos() - dir_light_cam_offset * glm::normalize(dir_light.direction),
            engine.camera().get_pos(),
            globals::basis.y()
        );

        light_pv = light_projection * light_view;

        sp_dir_depth_.use()
            .and_then_with_self([&, this] (ActiveShaderProgram& ashp) {

                dir_light_shadow_map.framebuffer().bind()
                    .and_then([&, this] {
                        glClear(GL_DEPTH_BUFFER_BIT);

                        ashp.uniform("projection", light_projection)
                            .uniform("view", light_view);

                        auto view = registry.view<Transform, Shared<Model>>();

                        for (auto [_, transform, model] : view.each()) {

                            ashp.uniform("model", transform.mtransform().model());

                            for (auto& drawable : model->drawable_meshes()) {
                                drawable.mesh().draw();
                            }
                        }
                    })
                    .unbind();

            });

    }

    return light_pv;
}




inline void MaterialDSMultilightShadowStage::draw_scene(
    const RenderEngine::PrimaryInterface& engine,
    const entt::registry& registry, const glm::mat4& dir_light_pv)
{
    using namespace gl;

    auto [w, h] = engine.window_size();
    glViewport(0, 0, w, h);

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

        // Dir light.
        // FIXME: I stopped caring about Locations here...
        for (auto [e, dir] : registry.view<const light::Directional>().each()) {
            ashp.uniform("dir_light.color", dir.color)
                .uniform("dir_light.direction", dir.direction)
                .uniform("dir_light_cast_shadows", registry.all_of<components::ShadowCasting>(e));
        }
        ashp.uniform("dir_light_pv", dir_light_pv)
            .uniform("dir_shadow_bias_bounds", dir_shadow_bias_bounds)
            .uniform("dir_light_pcf_samples", dir_light_pcf_samples)
            .uniform("dir_light_shadow_map", 2);
        dir_light_shadow_map.depth_target().bind_to_unit(GL_TEXTURE2);

        // Point light properties are sent through SSBOs.
        // Send the depth cubemap array for point light shadow calculation.
        ashp.uniform(locs_.point_light_shadow_maps, 3);
        plight_shadow_maps.depth_taget().bind_to_unit(GL_TEXTURE3);

        // Extra settings for point light shadows.
        ashp.uniform(locs_.point_light_z_far, plight_z_near_far.y)
            .uniform(locs_.point_shadow_bias_bounds, point_shadow_bias_bounds)
            .uniform("point_light_pcf_samples", point_light_pcf_samples)
            .uniform("point_light_pcf_offset", point_light_pcf_offset)
            .uniform("point_light_use_fixed_pcf_samples", point_light_use_fixed_pcf_samples);

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








class MaterialDSMultilightShadowStageImGuiHook {
private:
    MaterialDSMultilightShadowStage& stage_;
    int point_shadow_res_;
    int dir_shadow_res_;

public:
    MaterialDSMultilightShadowStageImGuiHook(MaterialDSMultilightShadowStage& stage)
        : stage_{ stage }
        , point_shadow_res_{
            stage_.plight_shadow_maps.width()
        }
        , dir_shadow_res_{
            stage_.dir_light_shadow_map.width()
        }
    {}


    void operator()() {
        auto& s = stage_;

        if (ImGui::TreeNode("Point Shadows")) {
            const bool unconfirmed_changes =
                s.plight_shadow_maps.width() != point_shadow_res_;

            const char* label =
                unconfirmed_changes ? "*Apply" : " Apply";

            if (ImGui::Button(label)) {
                s.plight_shadow_maps.reset_size(point_shadow_res_, point_shadow_res_, s.plight_shadow_maps.depth());
            }
            ImGui::SameLine();
            ImGui::SliderInt(
                "Resolution", &point_shadow_res_,
                128, 8192, "%d", ImGuiSliderFlags_Logarithmic
            );

            ImGui::SliderFloat2(
                "Z Near/Far", glm::value_ptr(s.plight_z_near_far),
                0.01f, 500.f, "%.3f", ImGuiSliderFlags_Logarithmic
            );

            ImGui::SliderFloat2(
                "Shadow Bias", glm::value_ptr(s.point_shadow_bias_bounds),
                0.00001f, 0.5f, "%.5f", ImGuiSliderFlags_Logarithmic
            );

            ImGui::Checkbox("Use Fixed PCF Samples", &s.point_light_use_fixed_pcf_samples);

            ImGui::BeginDisabled(s.point_light_use_fixed_pcf_samples);
            ImGui::SliderInt(
                "PCF Samples", &s.point_light_pcf_samples, 0, 6
            );
            ImGui::EndDisabled();

            ImGui::SliderFloat(
                "PCF Offset", &s.point_light_pcf_offset,
                0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic
            );

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Directional Shadows")) {
            if (ImGui::TreeNode("Shadow Map")) {

                ImGui::Image(
                    reinterpret_cast<ImTextureID>(
                        s.dir_light_shadow_map.depth_target().id()
                    ),
                    ImVec2{ 300.f, 300.f }
                );

                ImGui::TreePop();
            }

            const bool unconfirmed_changes =
                s.dir_light_shadow_map.width() != dir_shadow_res_;

            const char* label =
                unconfirmed_changes ? "*Apply" : " Apply";

            if (ImGui::Button(label)) {
                s.dir_light_shadow_map.reset_size(
                    dir_shadow_res_, dir_shadow_res_
                );
            }
            ImGui::SameLine();
            ImGui::SliderInt(
                "Resolution", &dir_shadow_res_,
                128, 8192, "%d", ImGuiSliderFlags_Logarithmic
            );

            ImGui::SliderFloat2(
                "Bias", glm::value_ptr(s.dir_shadow_bias_bounds),
                0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic
            );

            ImGui::SliderFloat(
                "Proj Scale", &s.dir_light_projection_scale,
                0.1f, 10000.f, "%.1f", ImGuiSliderFlags_Logarithmic
            );

            ImGui::SliderFloat2(
                "Z Near/Far", glm::value_ptr(s.dir_light_z_near_far),
                0.001f, 10000.f, "%.3f", ImGuiSliderFlags_Logarithmic
            );

            ImGui::SliderFloat(
                "Cam Offset", &s.dir_light_cam_offset,
                0.1f, 10000.f, "%.1f", ImGuiSliderFlags_Logarithmic
            );

            ImGui::SliderInt(
                "PCF Samples", &s.dir_light_pcf_samples, 0, 12
            );


            ImGui::TreePop();
        }

    }

};






} // namespace learn
