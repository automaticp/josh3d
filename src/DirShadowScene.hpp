#pragma once
#include "AssimpModelLoader.hpp"
#include "GLObjects.hpp"
#include "GlobalsUtil.hpp"
#include "ImGuiContextWrapper.hpp"
#include "Input.hpp"
#include "LightCasters.hpp"
#include "Model.hpp"
#include "Camera.hpp"
#include "ShaderBuilder.hpp"
#include "RenderTargetDepth.hpp"
#include "Transform.hpp"
#include <glbinding/gl/functions.h>
#include <glfwpp/window.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>
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





}

using leakslearn::DirShadowScene;
