#pragma once
#include "All.hpp"
#include "AssimpModelLoader.hpp"
#include "GLObjects.hpp"
#include "Globals.hpp"
#include "Input.hpp"
#include "CubemapData.hpp"
#include "LightCasters.hpp"
#include "SkyboxRenderer.hpp"
#include "Transform.hpp"
#include "ImGuiContextWrapper.hpp"
#include <glfwpp/window.h>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>



namespace leakslearn {

using namespace learn;

class ModelScene {
private:
    glfw::Window& window_;

    ShaderProgram shader_;

    Model model_;

    SkyboxRenderer sky_renderer_;
    Cubemap cubemap_;

    light::Ambient ambient_;
    light::Directional directional_;
    light::Point light_;

    Camera cam_;

    RebindableInputFreeCamera input_;

    ImGuiContextWrapper imgui_;

public:
    ModelScene(glfw::Window& window)
        : window_{ window }
        , shader_{
            ShaderBuilder()
                .load_vert("src/shaders/non_instanced.vert")
                .load_frag("src/shaders/mat_ds_light_ad.frag")
                .get()
        }
        , model_{
            AssimpModelLoader<>()
                .load("data/models/backpack/backpack.obj")
                .get()
        }
        , ambient_{
            .color = { 0.428f, 0.443f, 0.457f }
        }
        , directional_{
            .color = { 0.545f, 0.545f, 0.490f },
            .direction = { 0.45f, -0.45f, -0.77f }
        }
        , light_{
            .color = { 0.3f, 0.3f, 0.2f },
            .position = { 0.5f, 0.8f, 1.5f },
        }
        , cam_{ {0.0f, 0.0f, 3.0f}, {0.0f, 0.0f, -1.0f} }
        , input_{ window_, cam_ }
        , imgui_{ window_ }
    {
        CubemapData cubemap_data = CubemapData::from_files(
            {
                "data/textures/skybox/lake/right.png",
                "data/textures/skybox/lake/left.png",
                "data/textures/skybox/lake/top.png",
                "data/textures/skybox/lake/bottom.png",
                "data/textures/skybox/lake/front.png",
                "data/textures/skybox/lake/back.png",
            }
        );

        using namespace gl;
        cubemap_.bind()
            .attach_data(cubemap_data)
            .unbind();

        input_.bind_callbacks();
    }

    void process_input() {
        const bool ignore = ImGui::GetIO().WantCaptureKeyboard;
        input_.process_input(ignore);
    }

    void update() {}

    void render() {
        using namespace gl;
        imgui_.new_frame();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw_scene_objects();

        draw_gui();

        imgui_.render();
    }


private:
    void draw_scene_objects() {
        auto [width, height] = learn::globals::window_size.size();
        auto projection = glm::perspective(
            cam_.get_fov(),
            static_cast<float>(width) / static_cast<float>(height),
            0.1f, 100.0f
        );
        auto view = cam_.view_mat();

        glm::vec3 cam_pos{ cam_.get_pos() };


        sky_renderer_.draw(cubemap_, projection, view);

        auto transform = Transform();

        auto asp = shader_.use();
        asp.uniform("projection", projection);
        asp.uniform("view", view);
        asp.uniform("model", transform.model());
        asp.uniform("normal_model", transform.normal_model());

        asp.uniform("ambient_light.color", ambient_.color);
        asp.uniform("dir_light.color", directional_.color);
        asp.uniform("dir_light.direction", directional_.direction);

        // asp.uniform("cam_pos", cam_pos);

        model_.draw(asp);

    }



    void draw_gui() {
        static std::string filepath{ "data/models/backpack/backpack.obj" };
        static ImVec2 window_scale{ 55.f, 15.f };
        static ImVec2 window_size{
            window_scale.x * ImGui::GetFontSize(),
            window_scale.y * ImGui::GetFontSize()
        };

        ImGui::Begin("Debug");

        ImGui::SetWindowSize(window_size, ImGuiCond_Once);
        ImGui::SetWindowPos({ 0, 0 }, ImGuiCond_Once);

        ImGui::InputText("Path", &filepath);
        if (ImGui::Button("Load Model")) {
            try {
                model_ = learn::AssimpModelLoader<>()
                    .load(filepath)
                    .get();
            } catch (learn::error::AssimpLoaderError& e) {
                ImGui::LogText("%s", e.what());
            }
        }

        ImGui::ColorEdit3("Amb Color", glm::value_ptr(ambient_.color));

        ImGui::SliderFloat3("Dir Direction", glm::value_ptr(directional_.direction), -1.f, 1.f);
        ImGui::ColorEdit3("Dir Color", glm::value_ptr(directional_.color));

        glm::vec3 cam_dir = -cam_.back_uv();
        ImGui::Text("Cam Direction: (%.2f, %.2f, %.2f)", cam_dir.x, cam_dir.y, cam_dir.z);

        if (ImGui::Button("Face Light to Camera")) {
            directional_.direction = -cam_dir;
        }

        ImGui::End();

    }

};

} // namespace leakslearn

using leakslearn::ModelScene;
