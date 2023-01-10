#pragma once
#include "GLObjects.hpp"
#include "Vertex2D.hpp"


namespace learn {


class PostprocessRenderer {
private:
    VBO quad_vbo_{};
    VAO quad_vao_{};

public:
    PostprocessRenderer() {
        quad_vbo_.bind()
            .attach_data(quad.size(), quad.data(), gl::GL_STATIC_DRAW)
            .associate_with<Vertex2D>(quad_vao_.bind());
    }

    void draw(ActiveShaderProgram& pp_shader, TextureHandle& screen_color_texture) {
        using namespace gl;
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        // These hardcoded uniform name and unit number are concerning...
        screen_color_texture.bind_to_unit(GL_TEXTURE0);
        pp_shader.uniform("color", 0);

        quad_vao_.bind().draw_arrays(GL_TRIANGLES, 0, quad.size());
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
