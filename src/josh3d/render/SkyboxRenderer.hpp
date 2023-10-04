#pragma once
#include "GLObjects.hpp"
#include "ShaderBuilder.hpp"
#include "AttributeParams.hpp"
#include "VPath.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>

namespace josh {


// Pretty basic, one skybox at a time.
class SkyboxRenderer {
private:
    UniqueShaderProgram skybox_shader_;

    UniqueVBO cube_vbo_;
    UniqueVAO cube_vao_;

public:
    SkyboxRenderer()
        : skybox_shader_{
            ShaderBuilder()
                .load_vert(VPath("src/shaders/skybox.vert"))
                .load_frag(VPath("src/shaders/skybox.frag"))
                .get()
        }
    {
        using namespace gl;

        AttributeParams aparams[1]{{ 0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0 }};

        auto bvao = cube_vao_.bind();

        cube_vbo_.bind()
            .specify_data<glm::vec3>(skybox_vertices, GL_STATIC_DRAW)
            .associate_with(bvao, aparams)
            .unbind();

        bvao.unbind();
    }

    void draw(RawCubemap<GLConst> skybox_cubemap,
        const glm::mat4& projection, const glm::mat4& view)
    {
        using namespace gl;

        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);

        skybox_cubemap.bind_to_unit(GL_TEXTURE0);
        skybox_shader_.use()
            .uniform("projection", projection)
            .uniform("view", glm::mat4{ glm::mat3{ view } })
            .uniform("cubemap", 0)
            .and_then([this] {
                cube_vao_.bind()
                    .draw_arrays(GL_TRIANGLES, 0, skybox_vertices.size())
                    .unbind();
            });

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

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

} // namespace josh
