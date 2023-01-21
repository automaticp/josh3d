#pragma once
#include "AssimpModelLoader.hpp"
#include "GLObjects.hpp"
#include "Input.hpp"
#include "LightCasters.hpp"
#include "ShaderBuilder.hpp"
#include "Model.hpp"
#include "Transform.hpp"
#include "Camera.hpp"
#include "glfwpp/window.h"
#include <glbinding/gl/enum.h>
#include <glm/fwd.hpp>


namespace leakslearn {

using namespace learn;


class BoxScene {
private:
    glfw::Window& window_;

    ShaderProgram sp_box_;
    ShaderProgram sp_light_;

    Model<> box_;

    std::vector<Vertex> box_vertices_;

    VAO light_vao_;
    VBO light_vbo_;

    static const std::array<glm::vec3, 10> initial_box_positions;
    std::array<Transform, 10> box_transforms_;

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
        , box_vertices_{
            box_.mehses().at(0).vertices()
        }
        , box_transforms_{
            [] {
                std::array<Transform, 10> ts;
                for (size_t i{ 0 }; i < ts.size(); ++i) {
                    ts[i] = Transform()
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

        using namespace gl;
        light_vbo_.bind()
            .attach_data(box_vertices_.size(), box_vertices_.data(),GL_STATIC_DRAW)
            .associate_with<Vertex>(light_vao_.bind());

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

        auto bound_light_vao = light_vao_.bind();

        asp_light.uniform("projection", projection);
        asp_light.uniform("view", view);

        for (const auto& lp : lps_) {
            Transform lp_transform;
            lp_transform.translate(lp.position).scale(glm::vec3{ 0.2f });
            asp_light.uniform("model", lp_transform.model());

            // Will be compiled out regardless
            // asp_light.uniform("normalModel", lp_transform.normal_model());

            asp_light.uniform("lightColor", lp.color);

            bound_light_vao.draw_arrays(GL_TRIANGLES, 0, box_vertices_.size());
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

} // namespace leakslearn

using leakslearn::BoxScene;
