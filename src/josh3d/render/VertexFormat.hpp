#pragma once
#include "ContainerUtils.hpp"
#include "EnumUtils.hpp"
#include "Scalars.hpp"
#include "VertexSkinned.hpp"
#include "VertexStatic.hpp"


namespace josh {

/*
For now we use a simple fixed set of vertex formats.

This could be "upgraded" to an open compile-time defined set of layouts
with the help of the HashedString, similar to ResourceType, or even to
a fully runtime defined layout, although making shaders work with that
would be a major PITA given that they are currently 99% hand-written.
*/
enum class VertexFormat : u8
{
    Static,
    Skinned,
};
JOSH3D_DEFINE_ENUM_EXTRAS(VertexFormat, Static, Skinned);

template<VertexFormat V> struct vertex_type;
template<> struct vertex_type<VertexFormat::Static>  { using type = VertexStatic;  };
template<> struct vertex_type<VertexFormat::Skinned> { using type = VertexSkinned; };
template<VertexFormat V> using vertex_type_t = vertex_type<V>::type;

template<typename VertexT> struct vertex_format;
template<> struct vertex_format<VertexStatic>  : value_constant<VertexFormat::Static>  {};
template<> struct vertex_format<VertexSkinned> : value_constant<VertexFormat::Skinned> {};
template<typename VertexT> constexpr auto vertex_format_v = vertex_format<VertexT>::value;

} // namespace josh
