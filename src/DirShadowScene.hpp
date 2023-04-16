#pragma once
#include "AssimpModelLoader.hpp"
#include "GLObjects.hpp"
#include "GlobalsUtil.hpp"
#include "ImGuiContextWrapper.hpp"
#include "Input.hpp"
#include "LightCasters.hpp"
#include "MaterialDSDirLightShadowStage.hpp"
#include "Model.hpp"
#include "Camera.hpp"
#include "ShaderBuilder.hpp"
#include "RenderTargetDepth.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include "RenderEngine.hpp"
#include <glbinding/gl/functions.h>
#include <glfwpp/window.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <entt/entt.hpp>
#include <imgui.h>



namespace leakslearn {

using namespace learn;



class DirShadowScene {
private:
    glfw::Window& window_;

    ShaderProgram shader_;
    ShaderProgram depth_shader_;

    RenderTargetDepth depth_target_;

    Model model_;
    MTransform model_transform_;

    light::Ambient ambient_;
    light::Directional directional_;

    Camera cam_;

    RebindableInputFreeCamera input_;

    ImGuiContextWrapper imgui_;

    glm::vec2 shadow_bias_bounds_{ 0.0001f, 0.0015f };
    float light_projection_scale_{ 50.f };
    glm::vec2 light_z_near_far_{ 15.f, 150.f };
    float camera_offset_{ 100.f };


public:
    DirShadowScene(glfw::Window& window)
        : window_{ window }
        , shader_{
            ShaderBuilder()
                .load_vert("src/shaders/in_directional_shadow.vert")
                .load_frag("src/shaders/mat_ds_light_ad_shadow.frag")
                .get()
        }
        , depth_shader_{
            ShaderBuilder()
                .load_vert("src/shaders/depth_map.vert")
                .load_frag("src/shaders/depth_map.frag")
                .get()
        }
        , depth_target_{ 4096, 4096 }
        , model_{
            AssimpModelLoader<>()
                .load("data/models/shadow_scene/shadow_scene.obj")
                .get()
        }
        , ambient_{
            .color = { 0.4f, 0.4f, 0.4f }
        }
        , directional_{
            .color = { 0.5f, 0.5f, 0.5f },
            .direction = { -0.45f, -0.45f, -0.77f }
        }
        , cam_{
            { 0.f, 0.f, 3.f }, { 0.f, 0.f, -1.f }
        }
        , input_{ window_, cam_ }
        , imgui_{ window_ }
    {

        input_.use();

    }

    void process_input() {
        const bool ignore = ImGui::GetIO().WantCaptureKeyboard;
        input_.process_input(ignore);
    }

    void update() {}

    void render() {
        using namespace gl;

        glClearColor(
            ambient_.color.r, ambient_.color.g,
            ambient_.color.b, 1.0f
        );
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        imgui_.new_frame();

        glm::mat4 light_projection = glm::ortho(
            -light_projection_scale_, light_projection_scale_,
            -light_projection_scale_, light_projection_scale_,
            light_z_near_far_.x, light_z_near_far_.y
        );

        glm::mat4 light_view = glm::lookAt(
            cam_.get_pos() - camera_offset_ * glm::normalize(directional_.direction),
            cam_.get_pos(),
            // -directional_.direction,
            // glm::vec3(0.f),
            globals::basis.y()
        );

        glViewport(0, 0, depth_target_.width(), depth_target_.height());
        depth_target_.framebuffer().bind()
            .and_then([&] {
                glClear(GL_DEPTH_BUFFER_BIT);
                draw_scene_depth(light_projection, light_view);
            })
            .unbind();


        auto [w, h] = globals::window_size.size();
        glViewport(0, 0, w, h);

        draw_scene_objects(light_projection, light_view);

        draw_gui();
        imgui_.render();
    }

private:
    void draw_scene_objects(const glm::mat4& light_projection, const glm::mat4& light_view) {
        using namespace gl;

        auto [width, height] = globals::window_size.size<float>();
        glm::mat4 projection = cam_.perspective_projection_mat(width / height);
        glm::mat4 view = cam_.view_mat();

        auto asp = shader_.use();

        asp .uniform("projection", projection)
            .uniform("view", view)
            .uniform("model", model_transform_.model())
            .uniform("normal_model", model_transform_.normal_model());

        asp .uniform("dir_light_pv", light_projection * light_view);

        depth_target_.depth_target().bind_to_unit(GL_TEXTURE2);
        asp .uniform("shadow_map", 2);

        asp .uniform("ambient_light.color", ambient_.color)
            .uniform("dir_light.direction", directional_.direction)
            .uniform("dir_light.color", directional_.color);

        asp .uniform("shadow_bias_bounds", shadow_bias_bounds_);

        model_.draw(asp);

    }

    void draw_scene_depth(const glm::mat4& projection, const glm::mat4& view) {

        auto asp = depth_shader_.use();

        asp.uniform("projection", projection);
        asp.uniform("view", view);
        asp.uniform("model", model_transform_.model());

        model_.draw(asp);

    }


    void draw_gui() {

        static int shadow_res{ depth_target_.width() };

        ImGui::Begin("Debug");

        ImGui::Text("Shadow Depth Buffer");
        ImGui::Image(
            reinterpret_cast<ImTextureID>(depth_target_.depth_target().id()),
            ImVec2{ 512.f, 512.f }
        );

        ImGui::SliderFloat3("Light Dir", glm::value_ptr(directional_.direction), -1.f, 1.f);

        if (ImGui::Button("Set Dir to Camera")) {
            directional_.direction = -cam_.back_uv();
        }

        ImGui::SliderInt("Shadow Resolution", &shadow_res, 128, 8192, "%d", ImGuiSliderFlags_Logarithmic);
        if (ImGui::Button("Apply Resolution")) {
            depth_target_.reset_size(shadow_res, shadow_res);
        }

        ImGui::SliderFloat2("Bias", glm::value_ptr(shadow_bias_bounds_), 0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);

        ImGui::SliderFloat("Proj Scale", &light_projection_scale_, 0.1f, 10000.f, "%.1f", ImGuiSliderFlags_Logarithmic);

        ImGui::SliderFloat2("Z Near Far", glm::value_ptr(light_z_near_far_), 0.001f, 10000.f, "%.3f", ImGuiSliderFlags_Logarithmic);

        ImGui::SliderFloat("Cam Offset", &camera_offset_, 0.1f, 10000.f, "%.1f", ImGuiSliderFlags_Logarithmic);

        ImGui::End();

    }




};


class DirShadowScene2 {
private:
    glfw::Window& window_;

    entt::registry registry_;
    Camera cam_{ { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } };
    RebindableInputFreeCamera input_{ window_, cam_ };

    RenderEngine rengine_{ registry_, cam_, globals::window_size.size_ref() };
    ImGuiContextWrapper imgui_{ window_ };

    MaterialDSDirLightShadowStage* shadow_stage_{};
    entt::entity ambi_light_entity_;
    entt::entity dir_light_entity_;

public:
    DirShadowScene2(glfw::Window& window)
        : window_{ window }
    {
        input_.use();
        rengine_.stages().emplace_back(MaterialDSDirLightShadowStage());
        shadow_stage_ =
            rengine_.stages().back().target<MaterialDSDirLightShadowStage>();
        assert(shadow_stage_);
        init_registry();
    }

    void process_input() {
        const bool ignore = ImGui::GetIO().WantCaptureKeyboard;
        input_.process_input(ignore);
    }

    void update() {}

    void render() {
        using namespace gl;

        auto ambicolor =
            registry_.get<light::Ambient>(ambi_light_entity_).color;
        glClearColor(
            ambicolor.r, ambicolor.g,
            ambicolor.b, 1.0f
        );
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        imgui_.new_frame();

        rengine_.render();

        update_gui();

        imgui_.render();
    }


private:
    void init_registry() {
        auto& r = registry_;

        auto loader = AssimpModelLoader<>();

        Shared<Model> model = std::make_shared<Model>(
            loader.load("data/models/shadow_scene/shadow_scene.obj").get()
        );

        auto e = r.create();
        r.emplace<Shared<Model>>(e, std::move(model));
        r.emplace<Transform>(e);

        Shared<Model> box = std::make_shared<Model>(
            loader.load("data/models/container/container.obj").get()
        );

        e = r.create();
        r.emplace<Transform>(e, Transform()
            .translate({ 1.f, 0.5f, -1.f })
        );
        r.emplace<Shared<Model>>(e, box);

        ambi_light_entity_ = r.create();
        r.emplace<light::Ambient>(ambi_light_entity_, light::Ambient{
            .color = { 0.15f, 0.15f, 0.1f }
        });

        dir_light_entity_ = r.create();
        r.emplace<light::Directional>(dir_light_entity_, light::Directional{
            .color = { 0.3f, 0.3f, 0.2f },
            .direction = { -0.2f, -1.0f, -0.3f }
        });
    }

    void update_gui() {

        auto& stage = *shadow_stage_;
        auto& dir_light = registry_.get<light::Directional>(dir_light_entity_);
        auto& ambi_light = registry_.get<light::Ambient>(ambi_light_entity_);

        static int shadow_res{ stage.depth_target.width() };

        ImGui::Begin("Debug");

        ImGui::Text("Shadow Depth Buffer");
        ImGui::Image(
            reinterpret_cast<ImTextureID>(
                stage.depth_target.depth_target().id()
            ),
            ImVec2{ 512.f, 512.f }
        );

        ImGui::ColorEdit3("Ambient Light Color", glm::value_ptr(ambi_light.color));

        ImGui::ColorEdit3("Dir Light Color", glm::value_ptr(dir_light.color));


        ImGui::SliderFloat3("Light Dir", glm::value_ptr(dir_light.direction), -1.f, 1.f);

        if (ImGui::Button("Set Dir to Camera")) {
            dir_light.direction = -cam_.back_uv();
        }

        ImGui::SliderInt("Shadow Resolution", &shadow_res, 128, 8192, "%d", ImGuiSliderFlags_Logarithmic);
        if (ImGui::Button("Apply Resolution")) {
            stage.depth_target.reset_size(shadow_res, shadow_res);
        }

        ImGui::SliderFloat2(
            "Bias", glm::value_ptr(stage.shadow_bias_bounds),
            0.0001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat(
            "Proj Scale", &stage.light_projection_scale,
            0.1f, 10000.f, "%.1f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat2(
            "Z Near Far", glm::value_ptr(stage.light_z_near_far),
            0.001f, 10000.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat(
            "Cam Offset", &stage.camera_offset,
            0.1f, 10000.f, "%.1f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::End();

        ImGui::Begin("Transform");

        for (auto [entity, transform] : registry_.view<Transform>().each()) {

            auto entity_val = static_cast<entt::id_type>(entity);

            ImGui::PushID(entity_val);
            ImGui::Text("Entity: %d", entity_val);

            ImGui::SliderFloat3(
                "Pos", glm::value_ptr(transform.position()),
                -10.f, 10.f
            );

            ImGui::SliderFloat3(
                "Scale", glm::value_ptr(transform.scaling()),
                0.1f, 10.f, "%.3f", ImGuiSliderFlags_Logarithmic
            );
            ImGui::PopID();

        }

        ImGui::End();
    }


};




}

using leakslearn::DirShadowScene;
using leakslearn::DirShadowScene2;
