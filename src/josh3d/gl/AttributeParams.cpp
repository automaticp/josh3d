#include "AttributeParams.hpp"
#include "Vertex2D.hpp"
#include "VertexPNT.hpp"
#include "VertexPNTTB.hpp"
#include "glbinding/gl/enum.h"


namespace josh {


const attribute_traits<Vertex2D>::params_type attribute_traits<Vertex2D>::aparams{
    {0u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex2D), offsetof(Vertex2D, position)},
    {1u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex2D), offsetof(Vertex2D, tex_uv)}
};


const attribute_traits<VertexPNT>::params_type attribute_traits<VertexPNT>::aparams{
    { 0u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNT), offsetof(VertexPNT, position) },
    { 1u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNT), offsetof(VertexPNT, normal) },
    { 2u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNT), offsetof(VertexPNT, tex_uv) }
};


const attribute_traits<VertexPNTTB>::params_type attribute_traits<VertexPNTTB>::aparams{
    { 0u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTTB), offsetof(VertexPNTTB, position) },
    { 1u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTTB), offsetof(VertexPNTTB, normal) },
    { 2u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTTB), offsetof(VertexPNTTB, tex_uv) },
    { 3u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTTB), offsetof(VertexPNTTB, tangent) },
    { 4u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(VertexPNTTB), offsetof(VertexPNTTB, bitangent) }
};


} // namespace josh
