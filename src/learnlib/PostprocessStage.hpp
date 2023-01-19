#pragma once
#include "All.hpp"
#include "ShaderBuilder.hpp"
#include "ShaderSource.hpp"
#include <array>
#include <string>
#include <glbinding/gl/bitfield.h>

namespace learn {



// The fragment shader must declare and use the following:
//
//     in vec2 tex_coords;
//     uniform sampler2D color;
//
class PostprocessStage {
private:
    VBO quad_vbo_{};
    VAO quad_vao_{};

    // This not ideal.
    // The shaders should be decoupled from the renderer,
    // and passed as the argument to draw_...() methods.
    // But for simplicity it's implemented this way for now.
    //
    // See PostprocessRenderer for a rough idea.
    ShaderProgram shader_;

    void init_buffers() {
        quad_vbo_.bind()
            .attach_data(quad.size(), quad.data(), gl::GL_STATIC_DRAW)
            .associate_with<Vertex2D>(quad_vao_.bind());
    }

public:
    explicit PostprocessStage(const std::string& frag_path)
        : shader_{
            ShaderBuilder()
                .load_vert("src/shaders/postprocess.vert")
                .load_frag(frag_path)
                .get()
        }
    {
        init_buffers();
    }

    explicit PostprocessStage(const ShaderSource& frag_source)
        : shader_{
            ShaderBuilder()
                .load_vert("src/shaders/postprocess.vert")
                .add_frag(frag_source)
                .get()
        }
    {
        init_buffers();
    }


    void draw(TextureHandle& color_tex) {
        using namespace gl;
        glDisable(GL_DEPTH_TEST);

        ActiveShaderProgram asp{ shader_.use() };

        color_tex.bind_to_unit(GL_TEXTURE0);

        // Might query the location of the uniform beforehand
        asp.uniform("color", 0);

        quad_vao_.bind()
            .draw_arrays(GL_TRIANGLES, 0, quad.size())
            .unbind();
    }

    void enable() {

    }


private:
    // Widning order must be counter-clockwise
    // so that the faces would not be culled.
    constexpr static std::array<Vertex2D, 6> quad{{
        { { +1.0f, -1.0f }, { 1.0f, 0.0f } },
        { { -1.0f, +1.0f }, { 0.0f, 1.0f } },
        { { -1.0f, -1.0f }, { 0.0f, 0.0f } },

        { { +1.0f, +1.0f }, { 1.0f, 1.0f } },
        { { -1.0f, +1.0f }, { 0.0f, 1.0f } },
        { { +1.0f, -1.0f }, { 1.0f, 0.0f } },
    }};


};



} // namespace learn
