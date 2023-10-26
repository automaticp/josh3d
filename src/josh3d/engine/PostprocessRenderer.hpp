#pragma once
#include "DefaultResources.hpp"
#include <glbinding/gl/gl.h>


namespace josh {


/*
Maybe this shouldn't exist?

This ended up un this hilarious state over the course of multiple refactors...
*/
class PostprocessRenderer {
public:
    // Disable depth-test and emit a draw call of a simple quad covering the entire screen.
    // Make sure all the shader uniforms and buffers are set up before calling this.
    void draw() {
        using namespace gl;
        glDisable(GL_DEPTH_TEST);
        globals::quad_primitive_mesh().draw();
    }

};


} // namespace josh
