#pragma once
#include "VertexConcept.hpp"
#include "AttributeParams.hpp"
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <cstddef>


namespace josh {


struct Vertex2D {
    glm::vec2 position;
    glm::vec2 tex_uv;

    using attributes_type = AttributeParams[2];
    static const attributes_type& get_attributes() noexcept { return aparams; }
    static const attributes_type aparams;
};




inline const Vertex2D::attributes_type Vertex2D::aparams{
    {0u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex2D), offsetof(Vertex2D, position)},
    {1u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex2D), offsetof(Vertex2D, tex_uv)}
};


static_assert(vertex<Vertex2D>);


} // namespace josh
