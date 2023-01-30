#pragma once
#include "CubemapData.hpp"
#include "GLObjects.hpp"
#include "Input.hpp"
#include "SkyboxRenderer.hpp"
#include <glfwpp/window.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>



namespace leakslearn {

using namespace learn;


class CubemapScene {
private:
    glfw::Window& window_;

    SkyboxRenderer sky_renderer_;

    Cubemap cubemap1_;
    Cubemap cubemap2_;
    bool is_first_cubemap_{ true };

    Camera cam_;

    RebindableInputFreeCamera input_;

public:
    CubemapScene(glfw::Window& window)
        : window_{ window }
        , cam_{}
        , input_{ window_, cam_ }
    {

        input_.set_keybind(
            glfw::KeyCode::N,
            [this] (const KeyCallbackArgs& args) {
                if (args.state == glfw::KeyState::Release) {
                    is_first_cubemap_ = !is_first_cubemap_;
                }
            }
        );

        input_.use();

        CubemapData data1 = CubemapData::from_files(
            {
                "data/textures/skybox/lake/right.png",
                "data/textures/skybox/lake/left.png",
                "data/textures/skybox/lake/top.png",
                "data/textures/skybox/lake/bottom.png",
                "data/textures/skybox/lake/front.png",
                "data/textures/skybox/lake/back.png",
            }
        );

        CubemapData data2 = CubemapData::from_files(
            {
                "data/textures/skybox/yokohama/posx.png",
                "data/textures/skybox/yokohama/negx.png",
                "data/textures/skybox/yokohama/posy.png",
                "data/textures/skybox/yokohama/negy.png",
                "data/textures/skybox/yokohama/posz.png",
                "data/textures/skybox/yokohama/negz.png",
            }
        );

        using namespace gl;

        cubemap1_.bind_to_unit(GL_TEXTURE0)
            .attach_data(data1)
            .unbind();

        cubemap2_.bind_to_unit(GL_TEXTURE0)
            .attach_data(data2)
            .unbind();

    }

    void process_input() {
        input_.process_input();
    }

    void update() {}

    void render() {

        Cubemap& active_cubemap = is_first_cubemap_ ? cubemap1_ : cubemap2_;

        using namespace gl;
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto [width, height] = learn::globals::window_size.size();

        glm::mat4 projection = glm::perspective(
            cam_.get_fov(),
            static_cast<float>(width) / static_cast<float>(height),
            0.1f, 100.0f
        );

        glm::mat4 view = cam_.view_mat();

        sky_renderer_.draw(active_cubemap, projection, view);

    }

};

} // namespace leakslearn

using leakslearn::CubemapScene;
