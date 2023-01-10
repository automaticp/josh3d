#pragma once
#include "All.hpp"
#include "AssimpModelLoader.hpp"
#include "Input.hpp"
#include "LightCasters.hpp"
#include "Transform.hpp"
#include "glfwpp/window.h"
#include <assimp/postprocess.h>


namespace leakslearn {

using namespace learn;

class ModelScene {
private:
    glfw::Window& window_;

    ShaderProgram shader_;

    Model<> backpack_model_;

    light::Point light_;

    Camera cam_;

    RebindableInputFreeCamera input_;

public:
    ModelScene(glfw::Window& window)
        : window_{ window }
        , shader_{ load_shader() }
        , backpack_model_{
            AssimpModelLoader<>()
                .add_flags(
                    aiProcess_OptimizeMeshes |
                    aiProcess_OptimizeGraph
                )
                .load("data/models/backpack/backpack.obj")
                .get()
        }
        , light_{
            .color = { 0.3f, 0.3f, 0.2f },
		    .position = { 0.5f, 0.8f, 1.5f },
        }
        , cam_{ {0.0f, 0.0f, 3.0f}, {0.0f, 0.0f, -1.0f} }
        , input_{ window_, cam_ }
    {
        input_.set_keybind(
            glfw::KeyCode::R,
            [this](const KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Release) {
                    shader_ = load_shader();
                }
            }
        );
        input_.set_keybind(
            glfw::KeyCode::T,
            [this](const KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Release) {
                    shader_ = load_shader_and_mangle();
                }
            }
        );

        input_.bind_callbacks();
    }

    void process_input() {
        input_.process_input();
    }

    void update() {}

    void render() {
        using namespace gl;
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw_scene_objects();
    }


private:
    void draw_scene_objects() {
        auto [width, height] = window_.getSize();
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

		asp.uniform("time", static_cast<float>(globals::frame_timer.current()));

		backpack_model_.draw(asp);

    }

    static ShaderProgram load_shader_and_mangle() {
        ShaderSource vert_src = ShaderSource::from_file("src/shaders/VertexShader.vert");
        vert_src.find_and_insert_as_next_line(
            "uniform",
            "uniform float time;"
        );

        vert_src.find_and_replace(
            "texCoord = aTexCoord;",
            "texCoord = cos(time) * aTexCoord;"
        );

        return ShaderBuilder()
            .add_vert(vert_src)
            .load_frag("src/shaders/TextureMaterialObject.frag")
            .get();
    }

    static ShaderProgram load_shader() {
        return ShaderBuilder()
            .load_vert("src/shaders/VertexShader.vert")
            .load_frag("src/shaders/TextureMaterialObject.frag")
            .get();
    }

};

} // namespace leakslearn

using leakslearn::ModelScene;
