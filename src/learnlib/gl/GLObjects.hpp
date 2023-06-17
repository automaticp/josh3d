#pragma once
#include "GLBuffers.hpp"
#include "GLShaders.hpp"
#include "GLTextures.hpp"
#include "GLFramebuffers.hpp"



namespace learn {


/*
This file defines thin wrappers around various OpenGL objects: Buffers, Arrays, Textures, Shaders, etc.
Each GL object wrapper defines a 'Bound' dummy nested class that permits actions,
which are only applicable to bound or in-use GL objects.

Bound dummies do not perform any sanity checks for actually being bound or being used in correct context.
Their lifetimes do not end when the parent object is unbound.
Use-after-unbinding is still a programmer error.
It's recommended to use them as rvalues whenever possible; their methods support chaining.

The interface of Bound dummies serves as a guide for establising dependencies
between GL objects and correct order of calls to OpenGL API.

The common pattern for creating a Vertex Array (VAO) with a Vertex Buffer (VBO)
and an Element Buffer (EBO) attached in terms of these wrappes looks like this:

    VAO vao;
    VBO vbo;
    EBO ebo;
    auto bvao = vao.bind();
    vbo.bind().attach_data(...).associate_with(bvao, attribute_layout);
    ebo.bind(bvao).attach_data(...);
    bvao.unbind();

From the example above you can infer that the association between VAO and VBO is made
during the VBO::associate_with(...) call (glVertexAttribPointer(), in particular),
whereas the EBO is associated with a currently bound VAO when it gets bound itself.

The requirement to pass a reference to an existing VAO::Bound dummy during these calls
also implies their dependency on the currently bound Vertex Array. It would not make
sense to make these calls in absence of a bound VAO.
*/


class TextureData;
class CubemapData;

/*
In order to not move trivial single-line definitions into a .cpp file
and to not have to prepend every OpenGL type and function with gl::,
we're 'using namespace gl' inside of 'leaksgl' namespace,
and then reexpose the symbols back to this namespace at the end
with 'using leaksgl::Type' declarations.
*/

namespace leaksgl {

using namespace gl;

class VAO;
class VBO;
class EBO;
class BoundVAO;
class BoundVBO;
class BoundEBO;



} // namespace leaksgl


} // namespace learn
