#pragma once
#include "All.hpp"
#include "AssimpModelLoader.hpp"
#include "BatchedShader.hpp"
#include "GLObjects.hpp"
#include "Globals.hpp"
#include "Input.hpp"
#include "LightCasters.hpp"
#include "Logging.hpp"
#include "MaterialDS.hpp"
#include "MaterialTraits.hpp"
#include "PostprocessDoubleBuffer.hpp"
#include "PostprocessStage.hpp"
#include "ShaderBuilder.hpp"
#include "TextureMSRenderTarget.hpp"
#include "TextureRenderTarget.hpp"
#include "glfwpp/window.h"
#include <array>
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <span>
#include <vector>


namespace leakslearn {

using namespace learn;

class PostprocessingScene {
private:
    glfw::Window& window_;

    Model box_;
    Model plane_;

    BatchedShader solid_shader_;

    light::Directional light_;

    Camera cam_;

    RebindableInputFreeCamera input_;

    PostprocessDoubleBuffer pdb_;
    TextureMSRenderTarget tex_ms_target_;

    bool use_msaa_{ true };
    bool use_pp_{ true };

    std::vector<PostprocessStage> pp_stages_;

public:
    PostprocessingScene(glfw::Window& window)
        : window_{ window }
        , box_{
            AssimpModelLoader<>()
                .load("data/models/container/container.obj").get()
        }
        , plane_{
            AssimpModelLoader<>()
                .load("data/models/plane/plane.obj").get()
        }
        , solid_shader_{
            ShaderBuilder()
                .load_vert("src/shaders/VertexShader.vert")
                .load_frag("src/shaders/MultiLightObject.frag")
                .get()
        }
        , light_{
            { 1.0f, 1.0f, 1.0f },
            glm::normalize(glm::vec3{ 0.2f, 0.5f, -0.8f })
        }
        , cam_{
            glm::vec3(0.0f, 0.0f, 3.0f),
            glm::vec3(0.0f, 0.0f, -1.0f)
        }
        , input_{ window, cam_ }
        , pdb_{
            learn::globals::window_size.width(),
            learn::globals::window_size.height()
        }
        , tex_ms_target_{
            learn::globals::window_size.width(),
            learn::globals::window_size.height(),
            8
        }
    {
        using namespace gl;

        window_.framebufferSizeEvent.setCallback(
            [this](glfw::Window&, int w, int h) {
                learn::globals::window_size.set_to(w, h);
                glViewport(0, 0, w, h);
                pdb_.reset_size(w, h);
                tex_ms_target_.reset_size_and_samples(w, h, use_msaa_ ? 8 : 1);
            }
        );

        input_.set_keybind(
            glfw::KeyCode::M,
            [this](const KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Release) {
                    auto [w, h] = learn::globals::window_size.size();
                    use_msaa_ = !use_msaa_;
                    tex_ms_target_.reset_size_and_samples(w, h, use_msaa_ ? 8 : 1);
                }
            }
        );

        input_.set_keybind(
            glfw::KeyCode::T,
            [this](const KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Release) {
                    use_pp_ = !use_pp_;
                }
            }
        );


        input_.bind_callbacks();

        for (
            auto&& path : {
                "src/shaders/pp_none.frag",
                // "src/shaders/pp_kernel_edge_circular.frag",
                // "src/shaders/pp_kernel_edge.frag",
                "src/shaders/pp_invert.frag",
                "src/shaders/pp_grayscale.frag",
                "src/shaders/pp_kernel_blur.frag",
                "src/shaders/pp_kernel_sharpen.frag",
                // "src/shaders/pp_test_cut_red.frag",
                // "src/shaders/pp_test_cut_green.frag"
            }
        )
        {
            pp_stages_.emplace_back(std::string(path));
        }
    }


    void process_input() {
        input_.process_input();
    }

    void update() {
        // what?
    }

    void render() {

        using namespace gl;


        // Begin drawing to the multisampled texture
        tex_ms_target_.framebuffer()
            .bind_as(GL_DRAW_FRAMEBUFFER)
            .and_then([&] {

                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);

                // Do the drawing
                draw_scene_objects();

            })
            .unbind();

        // Then blit from the multisampled buffer to the postprocess backbuffer

        pdb_.back().framebuffer()
            .bind_as(GL_DRAW_FRAMEBUFFER)
            .and_then([&] {

                auto [w, h] = learn::globals::window_size.size();
                tex_ms_target_.framebuffer()
                    .bind_as(GL_READ_FRAMEBUFFER)
                    .blit(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST)
                    .unbind();

            })
            .unbind();

        pdb_.swap_buffers();
        // This prepares the backbuffer with the initial render
        // and swaps it to the front.

        // Do the double-buffered postprocessing using PDB
        // with the Bind-Draw-Unbind-Swap loop

        if (use_pp_) {

            for (auto&& pp : std::span(pp_stages_.begin(), pp_stages_.end() - 1)) {
                pdb_.back().framebuffer()
                    .bind_as(GL_DRAW_FRAMEBUFFER)
                    .and_then([&] {
                        pp.draw(pdb_.front_target());
                    })
                    .unbind();
                pdb_.swap_buffers();
            }

            // Render last to the default framebuffer
            pp_stages_.back().draw(pdb_.front_target());

        } else {
            auto [w, h] = learn::globals::window_size.size();

            learn::BoundFramebuffer::unbind_as(GL_DRAW_FRAMEBUFFER);
            pdb_.front().framebuffer()
                .bind_as(GL_READ_FRAMEBUFFER)
                .blit(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST)
                .unbind();
        }

    }

private:
    void draw_scene_objects() {

        auto [width, height] = learn::globals::window_size.size();
        auto projection = glm::perspective(cam_.get_fov(), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
        auto view = cam_.view_mat();

        glm::vec3 cam_pos{ cam_.get_pos() };

        ActiveShaderProgram sasp{ solid_shader_.program().use() };

        auto& sp = solid_shader_;
        MaterialDSLocations locations = query_locations<MaterialDS>(sp.program());

        sp.uniform("projection", projection);
        sp.uniform("view", view);
        sp.uniform("camPos", cam_pos);

        sp.uniform("dirLight.color", light_.color);
        sp.uniform("dirLight.direction", light_.direction);

        sp.uniform("numPointLights", 0);

        auto box1_transform = Transform()
            .translate({1.0f, 1.0f, 0.5f});

        auto box2_transform = Transform()
            .translate({-1.0f, 1.0f, 0.5f})
            .rotate(glm::radians(45.f), { 0.f, 0.f, 1.f });

        auto plane_transform = Transform()
            .scale({ 5.f, 5.f, 1.f });


        sp.uniform("model", box1_transform.model());
        sp.uniform("normalModel", box1_transform.normal_model());

        box_.draw(sasp, locations);

        sp.uniform("model", box2_transform.model());
        sp.uniform("normalModel", box2_transform.normal_model());

        box_.draw(sasp, locations);

        sp.uniform("model", plane_transform.model());
        sp.uniform("normalModel", plane_transform.normal_model());

        plane_.draw(sasp, locations);



    }


};



} // namespace leakslearn

using leakslearn::PostprocessingScene;
