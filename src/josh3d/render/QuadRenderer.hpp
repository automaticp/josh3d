#pragma once
#include "GLBuffers.hpp"
#include "GLObjects.hpp"
#include "Vertex2D.hpp"
#include "GLScalars.hpp"
#include <glbinding/gl/enum.h>


namespace josh {


/*
Simple screen-filled quad render helper.

Input attributes:

    in vec2 pos;
    in vec2 tex_coords;

*/
class QuadRenderer {
private:
    VBO quad_vbo_{};
    VAO quad_vao_{};

public:
    QuadRenderer() {
        using enum GLenum;

        quad_vbo_.bind()
            .specify_data(quad.size(), quad.data(), GL_STATIC_DRAW)
            .associate_with<Vertex2D>(quad_vao_.bind());

    }

    // Emit a draw call on a quad.
    // No other change of state, depth-testing is not disabled.
    void draw() {
        using enum GLenum;
        quad_vao_.bind().draw_arrays(GL_TRIANGLES, 0, quad.size());
    }

private:
    // Winding order is counter-clockwise
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




} // namespace josh
