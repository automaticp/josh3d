#pragma once
#include "GLObjects.hpp"
#include "QuadRenderer.hpp"


namespace josh {


/*
Maybe this shouldn't exist?
*/
class PostprocessRenderer {
private:
    QuadRenderer renderer_;

public:
    void draw(ActiveShaderProgram& pp_shader, Texture2D& screen_color_texture) {
        using namespace gl;

        glDisable(GL_DEPTH_TEST);

        // These hardcoded uniform name and unit number are concerning...
        screen_color_texture.bind_to_unit(GL_TEXTURE0);
        pp_shader.uniform("color", 0);

        renderer_.draw();
    }

    // Emit a draw call of a simple quad covering the entire screen.
    // Make sure all the shader uniforms and buffers are set up before calling this.
    void draw() {
        using namespace gl;
        // FIXME: Whose job it is to disable depth testing?
        glDisable(GL_DEPTH_TEST);

        renderer_.draw();
    }

};



} // namespace josh
