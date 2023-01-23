#pragma once
#include "All.hpp"
#include "CubemapData.hpp"
#include "GLObjects.hpp"
#include "Input.hpp"
#include "Logging.hpp"
#include "ShaderBuilder.hpp"
#include "VertexTraits.hpp"
#include "glfwpp/window.h"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>



namespace leakslearn {

using namespace learn;


class CubemapScene {
private:
    glfw::Window& window_;

    ShaderProgram skybox_shader_;
    Cubemap cubemap1_;
    Cubemap cubemap2_;
    bool is_first_cubemap_{ true };

    VBO cube_vbo_;
    VAO cube_vao_;

    Camera cam_;

    RebindableInputFreeCamera input_;

public:
    CubemapScene(glfw::Window& window)
        : window_{ window }
        , skybox_shader_{
            ShaderBuilder()
                .load_vert("src/shaders/skybox.vert")
                .load_frag("src/shaders/skybox.frag")
                .get()
        }
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
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE)
            .unbind();

        cubemap2_.bind_to_unit(GL_TEXTURE0)
            .attach_data(data2)
            .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
            .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE)
            .unbind();

        cube_vbo_.bind()
            .attach_data(skybox_vertices.size(), skybox_vertices.data(), GL_STATIC_DRAW)
            .and_then([this] {
                auto bvao = cube_vao_.bind();
                bvao.set_attribute_params(AttributeParams{ 0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0 });
                bvao.enable_array_access(0)
                    .unbind();
            })
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


        glDepthMask(GL_FALSE);
        ActiveShaderProgram asp = skybox_shader_.use();
        asp .uniform("projection", projection)
            .uniform("view", view);

        active_cubemap.bind_to_unit(GL_TEXTURE0);
        asp.uniform("cubemap", 0);

        cube_vao_.bind()
            .draw_arrays(GL_TRIANGLES, 0, skybox_vertices.size())
            .unbind();

        glDepthMask(GL_TRUE);
    }




private:
    inline static const std::array<glm::vec3, 36> skybox_vertices{{
        {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},

        {-1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f},

        { 1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},

        {-1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f},

        {-1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f, -1.0f},

        {-1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f}
    }};

};

} // namespace leakslearn

using leakslearn::CubemapScene;
