#pragma once
#include "All.hpp"
#include "AssimpModelLoader.hpp"
#include "GLObjects.hpp"
#include "Globals.hpp"
#include "Input.hpp"
#include "Transform.hpp"
#include <glfwpp/window.h>


namespace leakslearn {

using namespace learn;

class InstancingScene {
private:
    glfw::Window& window_;

    ShaderProgram instanced_shader_;
    ShaderProgram non_instanced_shader_;
    SSBO instance_ssbo_;
    std::vector<Transform> instance_transforms_;

    Model<> box_model_;

    light::Point light_;

    Camera cam_;

    RebindableInputFreeCamera input_;

    bool is_instanced_{ true };

public:
    InstancingScene(glfw::Window& window)
        : window_{ window }
        , instanced_shader_{
            ShaderBuilder()
                .load_vert("src/shaders/instanced.vert")
                .load_frag("src/shaders/tex_ds_one_point_light.frag")
                .get()
        }
        , non_instanced_shader_{
            ShaderBuilder()
                .load_vert("src/shaders/non_instanced.vert")
                .load_frag("src/shaders/tex_ds_one_point_light.frag")
                .get()
        }
        , box_model_{
            AssimpModelLoader<>()
                .load("data/models/container/container.obj")
                .get()
        }
        , light_{
            .color = { 1.f, 1.0f, 0.8f },
            .position = { 2.5f, 2.8f, 100.f },
            .attenuation = { 0.0f, 0.0f, 0.001f }
        }
        , cam_{ { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } }
        , input_{ window_, cam_ }
    {

        input_.set_keybind(
            glfw::KeyCode::I,
            [this] (const KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Release) {
                    is_instanced_ = true;
                }
            }
        );

        input_.set_keybind(
            glfw::KeyCode::N,
            [this] (const KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Release) {
                    is_instanced_ = false;
                }
            }
        );

        input_.use();

        init_transforms();
        instance_ssbo_.bind_to(0)
            .attach_data(
                instance_transforms_.size(),
                instance_transforms_.data(),
                gl::GL_STATIC_DRAW
            )
            .unbind();
    }


    void process_input() {
        input_.process_input();
    }

    void update() {
        update_transforms();
    }

    void render() {
        using namespace gl;
        glClearColor(0.15f, 0.15f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_scene_objects();
    }


private:
    void draw_scene_objects() {

        // I and N to switch between
        // Instanced and Non-instanced modes.
        if (is_instanced_) {
            update_ssbo();
            draw_scene_instanced();
        }
        else {
            draw_scene_non_instanced();
        }

    }

    void draw_scene_instanced() {

        auto asp = instanced_shader_.use();

        set_common_uniforms(asp);

        box_model_.draw_instanced(asp, instance_transforms_.size());

    }


    void draw_scene_non_instanced() {
        auto asp = non_instanced_shader_.use();

        set_common_uniforms(asp);

        for (const auto& transform : instance_transforms_) {
            asp.uniform("model", transform.model());
            asp.uniform("normal_model", transform.normal_model());
            box_model_.draw(asp);
        }
    }

    void set_common_uniforms(ActiveShaderProgram& asp) {

        auto [width, height] = learn::globals::window_size.size();

        glm::mat4 projection = glm::perspective(
            cam_.get_fov(),
            static_cast<float>(width) / static_cast<float>(height),
            0.1f, 100.0f
        );

        asp .uniform("projection", projection)
            .uniform("view", cam_.view_mat())
            .uniform("cam_pos", cam_.get_pos());

        asp .uniform("point_light.color", light_.color)
            .uniform("point_light.position", light_.position)
            .uniform("point_light.attenuation.constant",
                light_.attenuation.constant)
            .uniform("point_light.attenuation.linear",
                light_.attenuation.linear)
            .uniform("point_light.attenuation.quadratic",
                light_.attenuation.quadratic);
    }

    void init_transforms() {

        constexpr size_t rows{ 50 };
        constexpr size_t cols{ 50 };

        instance_transforms_.reserve(rows * cols);
        for (size_t i{ 0 }; i < rows; ++i) {
            for (size_t j{ 0 }; j < cols; ++j) {
                const float x_offset = i * 2.f;
                const float y_offset = j * 2.f;

                instance_transforms_.emplace_back(
                    Transform().translate(
                        { x_offset, y_offset, 0.f }
                    )
                );
            }
        }
    }

    void update_transforms() {

        auto angle = learn::globals::frame_timer.delta<float>();
        for (auto& t : instance_transforms_) {
            t.rotate(angle, { 0.f, 0.f, 1.f });
        }

    }

    void update_ssbo() {

        instance_ssbo_.bind_to(0)
            .sub_data(instance_transforms_.size(), 0, instance_transforms_.data())
            .unbind();

    }

};


} // namespace leakslearn

using leakslearn::InstancingScene;
