#pragma once
#include "GLMutability.hpp"   // IWYU pragma: export
#include "GLBuffers.hpp"      // IWYU pragma: export
#include "GLVertexArray.hpp"  // IWYU pragma: export
#include "GLShaders.hpp"      // IWYU pragma: export
#include "GLTextures.hpp"     // IWYU pragma: export
#include "GLFramebuffer.hpp"  // IWYU pragma: export
#include "GLRenderbuffer.hpp" // IWYU pragma: export
#include "GLUnique.hpp"       // IWYU pragma: export


namespace josh {

#define JOSH3D_ALIAS_UNIQUE(object) \
    using object = GLUnique<Raw##object<GLMutable>>; // NOLINT(bugprone-macro-parentheses)

JOSH3D_ALIAS_UNIQUE(Texture2D)
JOSH3D_ALIAS_UNIQUE(Texture2DArray)
JOSH3D_ALIAS_UNIQUE(Texture2DMS)
JOSH3D_ALIAS_UNIQUE(Cubemap)
JOSH3D_ALIAS_UNIQUE(CubemapArray)

JOSH3D_ALIAS_UNIQUE(Framebuffer)
JOSH3D_ALIAS_UNIQUE(Renderbuffer)

JOSH3D_ALIAS_UNIQUE(VAO)

JOSH3D_ALIAS_UNIQUE(VBO)
JOSH3D_ALIAS_UNIQUE(EBO)
JOSH3D_ALIAS_UNIQUE(SSBO)
JOSH3D_ALIAS_UNIQUE(UBO)

JOSH3D_ALIAS_UNIQUE(Shader)
JOSH3D_ALIAS_UNIQUE(ShaderProgram)

#undef JOSH3D_ALIAS_UNIQUE


} // namespace josh


/*
Good description under construction.

TODO: This.
*/

