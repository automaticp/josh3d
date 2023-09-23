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

#define ALIAS_UNIQUE(object) \
    using object = GLUnique<Raw##object<GLMutable>>; // NOLINT(bugprone-macro-parentheses)

ALIAS_UNIQUE(Texture2D)
ALIAS_UNIQUE(Texture2DArray)
ALIAS_UNIQUE(Texture2DMS)
ALIAS_UNIQUE(Cubemap)
ALIAS_UNIQUE(CubemapArray)

ALIAS_UNIQUE(Framebuffer)
ALIAS_UNIQUE(Renderbuffer)

ALIAS_UNIQUE(VAO)

ALIAS_UNIQUE(VBO)
ALIAS_UNIQUE(EBO)
ALIAS_UNIQUE(SSBO)
ALIAS_UNIQUE(UBO)

ALIAS_UNIQUE(Shader)
ALIAS_UNIQUE(ShaderProgram)

#undef ALIAS_UNIQUE


} // namespace josh


/*
These files define thin wrappers around various OpenGL objects:
Buffers, Arrays, Textures, Shaders, etc.
Each GL object wrapper defines a 'Bound' dummy class that
permits actions, which are only applicable to bound or in-use GL objects.

Bound dummies do not perform any sanity checks for actually being bound
or being used in correct context. Their lifetimes do not end when
the parent object is unbound. Use-after-unbinding is still a programmer error.
It's recommended to use them as rvalues whenever possible; their methods support chaining.

The interface of Bound dummies serves as a guide for establising dependencies
between GL objects and correct order of calls to OpenGL API.

For example, a common pattern for creating a Vertex Array (VAO)
with a Vertex Buffer (VBO) and an Element Buffer (EBO) attached
in terms of these wrappes looks like this:

    auto bvao = vao.bind();
    vbo.bind().attach_data(...).associate_with(bvao, attribute_layout);
    ebo.bind(bvao).attach_data(...);
    bvao.unbind();

or like this:

    vao.bind()
        .and_then([&](BoundVAO& bvao) {
            vbo.bind()
                .associate_with(bvao, attribute_layout)
                .attach_data(...);

            ebo.bind(bvao)
                .attach_data(...);
        })
        .unbind();

From the example above you can infer that the association between VAO and VBO is made
during the BoundVBO::associate_with(...) call (glVertexAttribPointer(), in particular),
whereas the EBO is associated with a currently bound VAO when it gets bound itself.

The requirement to pass a reference to an existing BoundVAO dummy during these calls
also implies their dependency on the currently bound Vertex Array. It would not make
sense to make these calls in absence of a bound VAO.
*/

