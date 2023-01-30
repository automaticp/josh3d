#pragma once
#include "All.hpp"
#include "AssimpModelLoader.hpp"
#include "Globals.hpp"
#include "Input.hpp"
#include "LightCasters.hpp"
#include "Transform.hpp"
#include "ImGuiContextWrapper.hpp"
#include <glfwpp/window.h>
#include <assimp/postprocess.h>
#include <imgui.h>
#include <imgui_stdlib.h>



class ReloadModelGui {
private:
    std::string filepath_;
    learn::Model& model_ref_;
    ImVec2 window_scale_{ 55.f, 7.f };

public:
    ReloadModelGui(learn::Model& model)
        : model_ref_{ model }
    {}

    void process() {

        static ImVec2 window_size{
            window_scale_.x * ImGui::GetFontSize(),
            window_scale_.y * ImGui::GetFontSize()
        };

        ImGui::Begin("Load Model");


        ImGui::SetWindowSize(window_size, ImGuiCond_Once);
        ImGui::SetWindowPos({ 0, 0 }, ImGuiCond_Once);

        ImGui::InputText("Path", &filepath_);
        if (ImGui::Button("Load")) {
            try {
                model_ref_ = learn::AssimpModelLoader<>()
                    .load(filepath_)
                    .get();
            } catch (learn::error::AssimpLoaderError& e) {
                ImGui::LogText("%s", e.what());
            }
        }

        ImGui::End();


    }

};



namespace leakslearn {

using namespace learn;

class ModelScene {
private:
    glfw::Window& window_;

    ShaderProgram shader_;

    Model model_;

    light::Point light_;

    Camera cam_;

    RebindableInputFreeCamera input_;

    ImGuiContextWrapper imgui_;
    ReloadModelGui gui_;

public:
    ModelScene(glfw::Window& window)
        : window_{ window }
        , shader_{
            ShaderBuilder()
                .load_vert("src/shaders/VertexShader.vert")
                .load_frag("src/shaders/TextureMaterialObject.frag")
                .get()
        }
        , model_{
            AssimpModelLoader<>()
                .load("data/models/backpack/backpack.obj")
                .get()
        }
        , light_{
            .color = { 0.3f, 0.3f, 0.2f },
            .position = { 0.5f, 0.8f, 1.5f },
        }
        , cam_{ {0.0f, 0.0f, 3.0f}, {0.0f, 0.0f, -1.0f} }
        , input_{ window_, cam_ }
        , imgui_{ window_ }
        , gui_{ model_ }
    {
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

        gui_.process();

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


        auto backpack_transform = Transform();

        auto asp = shader_.use();
        asp.uniform("projection", projection);
        asp.uniform("view", view);
        asp.uniform("camPos", cam_pos);

        asp.uniform("model", backpack_transform.model());
        asp.uniform("normalModel", backpack_transform.normal_model());


        asp.uniform("lightColor", light_.color);
        asp.uniform("lightPos", light_.position);

        model_.draw(asp);

    }

};

} // namespace leakslearn

using leakslearn::ModelScene;
