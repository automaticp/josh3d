#pragma once
#include "AssimpModelLoader.hpp"
#include "GLObjects.hpp"
#include "ImGuiContextWrapper.hpp"
#include "Input.hpp"
#include "LightCasters.hpp"
#include "ShaderBuilder.hpp"
#include "Model.hpp"
#include "Transform.hpp"
#include "Camera.hpp"
#include "glfwpp/window.h"
#include <glbinding/gl/enum.h>
#include <glm/fwd.hpp>
#include <array>
#include <imgui.h>
#include <iterator>
#include <string>
#include <vector>


namespace leakslearn {

using namespace learn;


class BoxScene {
private:
    glfw::Window& window_;

    ShaderProgram sp_box_;
    ShaderProgram sp_light_;

    Model box_;

    static const std::array<glm::vec3, 10> initial_box_positions;
    std::array<MTransform, 10> box_transforms_;

    static const std::array<glm::vec3, 5> initial_point_light_positions_;
    std::array<light::Point, 5> lps_;

    light::Directional ld_;
    light::Spotlight ls_;

    Camera cam_;
    RebindableInputFreeCamera input_;


    bool is_flashlight_on_{ true };

public:
    BoxScene(glfw::Window& window)
        : window_{ window }
        , sp_box_{
            ShaderBuilder()
                .load_vert("src/shaders/VertexShader.vert")
                .load_frag("src/shaders/MultiLightObject.frag")
                .get()
        }
        , sp_light_{
            ShaderBuilder()
                .load_vert("src/shaders/VertexShader.vert")
                .load_frag("src/shaders/LightSource.frag")
                .get()
        }
        , box_{
            AssimpModelLoader<>()
                .load("data/models/container/container.obj")
                .get()
        }
        , box_transforms_{
            [] {
                std::array<MTransform, 10> ts;
                for (size_t i{ 0 }; i < ts.size(); ++i) {
                    ts[i] = MTransform()
                        .translate(initial_box_positions[i])
                        .rotate(glm::radians(20.0f * i), { 1.0f, 0.3f, 0.5f });
                }
                return ts;
            }()
        }
        , lps_{
            [] {
                std::array<light::Point, 5> lps;
                for (size_t i{ 0 }; i < lps.size(); ++i) {
                    lps[i] = light::Point{
                        .color = glm::vec3(1.0f, 1.f, 0.8f),
                        .position = initial_point_light_positions_[i],
                        .attenuation = light::Attenuation{
                            .constant = 1.f,
                            .linear = 0.4f,
                            .quadratic = 0.2f
                        }
                    };
                }
                return lps;
            }()
        }
        , ld_{
            .color = { 0.3f, 0.3f, 0.2f },
            .direction = { -0.2f, -1.0f, -0.3f }
        }
        , ls_{
            .color = glm::vec3(1.0f),
            .position = {},     // Update
            .direction = {},    // Update
            .attenuation = { .constant = 1.0f, .linear = 1.0f, .quadratic = 2.1f },
            .inner_cutoff_radians = glm::radians(12.0f),
            .outer_cutoff_radians = glm::radians(15.0f)
        }
        , cam_{
            glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f)
        }
        , input_{ window_, cam_ }
    {
        input_.set_keybind(
            glfw::KeyCode::F,
            [this](const KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Release) {
                    is_flashlight_on_ ^= true;
                    ls_.color = is_flashlight_on_ ? glm::vec3(1.0f) : glm::vec3(0.0f);
                }
            }
        );
        input_.use();

    }



    void process_input() {
        input_.process_input();
    }

    void update() {}

    void render() {
        using namespace gl;
        glClearColor(0.15f, 0.15f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw_scene_objects();
    }


private:
    void draw_scene_objects() {

        using namespace learn;
        using namespace gl;

        auto [width, height] = globals::window_size.size();
        glm::mat4 projection = glm::perspective(cam_.get_fov(), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
        glm::mat4 view = cam_.view_mat();

        glm::vec3 cam_pos{ cam_.get_pos() };


        ActiveShaderProgram asp{ sp_box_.use() };

        asp .uniform("projection", projection)
            .uniform("view", view)
            .uniform("camPos", cam_pos);


        // ---- Light Sources ----

        // Directional
        asp .uniform("dirLight.color", ld_.color)
            .uniform("dirLight.direction", ld_.direction);


        // Point

        asp.uniform("numPointLights", GLint(lps_.size()));

        for (size_t i{ 0 }; i < lps_.size(); ++i) {
            // what a mess, though
            std::string col_name{ "pointLights[x].color" };
            std::string pos_name{ "pointLights[x].position" };
            std::string att0_name{ "pointLights[x].attenuation.constant" };
            std::string att1_name{ "pointLights[x].attenuation.linear" };
            std::string att2_name{ "pointLights[x].attenuation.quadratic" };
            std::string i_string{ std::to_string(i) };
            col_name.replace(12, 1, i_string);
            pos_name.replace(12, 1, i_string);
            att0_name.replace(12, 1, i_string);
            att1_name.replace(12, 1, i_string);
            att2_name.replace(12, 1, i_string);

            asp.uniform(col_name, lps_[i].color);
            asp.uniform(pos_name, lps_[i].position);
            asp.uniform(att0_name, lps_[i].attenuation.constant);
            asp.uniform(att1_name, lps_[i].attenuation.linear);
            asp.uniform(att2_name, lps_[i].attenuation.quadratic);
        }

        // Spotlight
        ls_.position = cam_pos;
        ls_.direction = -cam_.back_uv();
        asp.uniform("spotLight.color", ls_.color);
        asp.uniform("spotLight.position", ls_.position);
        asp.uniform("spotLight.direction", ls_.direction);
        asp.uniform("spotLight.attenuation.constant", ls_.attenuation.constant);
        asp.uniform("spotLight.attenuation.linear", ls_.attenuation.linear);
        asp.uniform("spotLight.attenuation.quadratic", ls_.attenuation.quadratic);
        asp.uniform("spotLight.innerCutoffCos", glm::cos(ls_.inner_cutoff_radians));
        asp.uniform("spotLight.outerCutoffCos", glm::cos(ls_.outer_cutoff_radians));

        // ---- Scene of Boxes ----

        for (const auto& transform : box_transforms_) {
            asp.uniform("model", transform.model());
            asp.uniform("normalModel", transform.normal_model());
            box_.draw(asp);
        }


        // Point Lighting Sources

        ActiveShaderProgram asp_light{ sp_light_.use() };

        Mesh& box_mesh = box_.drawable_meshes().at(0).mesh();

        asp_light.uniform("projection", projection);
        asp_light.uniform("view", view);

        for (const auto& lp : lps_) {
            MTransform lp_transform;
            lp_transform.translate(lp.position).scale(glm::vec3{ 0.2f });
            asp_light.uniform("model", lp_transform.model());

            // Will be compiled out regardless
            // asp_light.uniform("normalModel", lp_transform.normal_model());

            asp_light.uniform("lightColor", lp.color);

            box_mesh.draw();
        }

    }


};



inline const std::array<glm::vec3, 10> BoxScene::initial_box_positions {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3( 2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3( 1.3f, -2.0f, -2.5f),
    glm::vec3( 1.5f,  2.0f, -2.5f),
    glm::vec3( 1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
};

inline const std::array<glm::vec3, 5> BoxScene::initial_point_light_positions_{
    glm::vec3( 0.7f,  0.2f,  2.0f),
    glm::vec3( 2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f,  2.0f, -12.0f),
    glm::vec3( 0.0f,  0.0f, -3.0f),
    glm::vec3( 0.0f,  1.0f,  0.0f)
};


// An iteration of box scene with new shaders.
// Instanced draws on boxes, dynamic number of point lights with SSBOs.
class BoxScene2 {
private:
    glfw::Window& window_;

    ShaderProgram sp_lit_model_;
    ShaderProgram sp_light_source_;

    Model box_;

    static const std::array<glm::vec3, 10> initial_box_positions;
    std::array<MTransform, 10> box_transforms_;
    SSBO box_transforms_ssbo_; // Model transform

    static const std::array<glm::vec3, 5> initial_point_light_positions_;

    std::array<light::Point, 5> lps_;
    SSBO lps_ssbo_; // Light description

    std::array<MTransform, 5> lps_transforms_;
    SSBO lps_transforms_ssbo_; // Model transform

    light::Directional ld_;
    light::Ambient la_;

    Camera cam_;
    RebindableInputFreeCamera input_;

    ImGuiContextWrapper imgui_;

public:
    BoxScene2(glfw::Window& window)
        : window_{ window }
        , sp_lit_model_{
            ShaderBuilder()
                .load_vert("src/shaders/instanced.vert")
                .load_frag("src/shaders/mat_ds_light_adpn.frag")
                .get()
        }
        , sp_light_source_{
            ShaderBuilder()
                .load_vert("src/shaders/instanced.vert")
                .load_frag("src/shaders/light_source.frag")
                .get()
        }
        , box_{
            AssimpModelLoader<>()
                .load("data/models/container/container.obj")
                .get()
        }
        , box_transforms_{
            [] {
                std::array<MTransform, 10> ts;
                for (size_t i{ 0 }; i < ts.size(); ++i) {
                    ts[i] = MTransform()
                        .translate(initial_box_positions[i])
                        .rotate(glm::radians(20.0f * i), { 1.0f, 0.3f, 0.5f });
                }
                return ts;
            }()
        }
        , lps_{
            [] {
                std::array<light::Point, 5> lps;
                for (size_t i{ 0 }; i < lps.size(); ++i) {
                    lps[i] = light::Point{
                        .color = glm::vec3(1.0f, 1.f, 0.8f),
                        .position = initial_point_light_positions_[i],
                        .attenuation = light::Attenuation{
                            .constant = 1.f,
                            .linear = 0.4f,
                            .quadratic = 0.2f
                        }
                    };
                }
                return lps;
            }()
        }
        , lps_transforms_{
            [this] {
                std::array<MTransform, 5> ts;
                for (size_t i{ 0 }; i < ts.size(); ++i) {
                    ts[i] = MTransform()
                        .translate(lps_[i].position)
                        .scale(glm::vec3{ 0.2f });
                }
                return ts;
            }()
        }
        , ld_{
            .color = { 0.3f, 0.3f, 0.2f },
            .direction = { -0.2f, -1.0f, -0.3f }
        }
        , la_{
            .color = { 0.15f, 0.15f, 0.1f }
        }
        , cam_{
            glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f)
        }
        , input_{ window_, cam_ }
        , imgui_{ window_ }
    {
        input_.use();

        init_ssbos();
    }



    void process_input() {
        bool block = ImGui::GetIO().WantCaptureKeyboard
            || ImGui::GetIO().WantCaptureMouse;
        input_.process_input(block);
    }

    void update() {
        update_transforms();
        update_point_light_transforms_ssbo();
        update_point_light_ssbo();
    }

    void render() {
        using namespace gl;

        glClearColor(la_.color.r, la_.color.g, la_.color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        imgui_.new_frame();

        draw_scene_objects();

        update_ui();
        imgui_.render();
    }


private:

    void draw_scene_objects() {
        using namespace gl;

        auto [width, height] = globals::window_size.size();
        glm::mat4 projection = glm::perspective(
            cam_.get_fov(),
            static_cast<float>(width) / static_cast<float>(height),
            0.1f, 100.0f
        );
        glm::mat4 view = cam_.view_mat();

        sp_lit_model_.use().and_then_with_self([&, this](ActiveShaderProgram& asp) {
            box_transforms_ssbo_.bind_to(0); // Instancing
            lps_ssbo_.bind_to(1); // Multiple lights

            asp .uniform("projection", projection)
                .uniform("view", view)
                .uniform("cam_pos", cam_.get_pos());

            asp. uniform("ambient_light.color", la_.color);

            asp .uniform("dir_light.color", ld_.color)
                .uniform("dir_light.direction", ld_.direction);

            box_.draw_instanced(asp, std::ssize(box_transforms_));
        });


        sp_light_source_.use().and_then_with_self([&, this](ActiveShaderProgram& asp) {
            lps_transforms_ssbo_.bind_to(0); // Instancing

            asp .uniform("projection", projection)
                .uniform("view", view);

            // Color is not per instance, so this is baaaaad
            asp .uniform("light_color", lps_[0].color);
            // Whatever

            Mesh& box_mesh = box_.drawable_meshes().at(0).mesh();

            box_mesh.draw_instanced(std::ssize(lps_transforms_));
        });

    }


    void init_ssbos() {
        using namespace gl;

        lps_ssbo_.bind()
            .attach_data(lps_.size(), lps_.data(), GL_STATIC_DRAW)
            .unbind();

        lps_transforms_ssbo_.bind()
            .attach_data(
                lps_transforms_.size(), lps_transforms_.data(),
                GL_STATIC_DRAW
            )
            .unbind();

        box_transforms_ssbo_.bind()
            .attach_data(
                box_transforms_.size(), box_transforms_.data(),
                GL_STATIC_DRAW
            )
            .unbind();

    }

    void update_transforms() {
        for (size_t i{ 0 }; i < lps_.size(); ++i) {
            lps_transforms_[i] = MTransform()
                .translate(lps_[i].position)
                .scale(glm::vec3{ 0.2f });
        }
    }

    void update_point_light_ssbo() {
        lps_ssbo_.bind()
            .sub_data(lps_.size(), 0, lps_.data())
            .unbind();
    }

    void update_point_light_transforms_ssbo() {
        lps_transforms_ssbo_.bind()
            .sub_data(lps_transforms_.size(), 0, lps_transforms_.data())
            .unbind();
    }

    void update_ui() {
        ImGui::Begin("Debug");

        size_t i{ 0 };
        light::Point& lp = lps_[0];
        // for (light::Point& lp : lps_) {
            ImGui::Text("Point Light %ld", ++i);
            ImGui::ColorEdit3("Color", glm::value_ptr(lp.color));
            ImGui::DragFloat3("Pos", glm::value_ptr(lp.position), 0.2f, -20.f, 20.f);
            ImGui::SliderFloat3("Attenuation", &lp.attenuation.constant, 0.f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        // }

        ImGui::End();
    }


};












inline const std::array<glm::vec3, 10> BoxScene2::initial_box_positions {
    glm::vec3( 0.0f,  0.0f,  0.0f),
    glm::vec3( 2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3( 2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3( 1.3f, -2.0f, -2.5f),
    glm::vec3( 1.5f,  2.0f, -2.5f),
    glm::vec3( 1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
};

inline const std::array<glm::vec3, 5> BoxScene2::initial_point_light_positions_{
    glm::vec3( 0.7f,  0.2f,  2.0f),
    glm::vec3( 2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f,  2.0f, -12.0f),
    glm::vec3( 0.0f,  0.0f, -3.0f),
    glm::vec3( 0.0f,  1.0f,  0.0f)
};











} // namespace leakslearn

using leakslearn::BoxScene;
using leakslearn::BoxScene2;
