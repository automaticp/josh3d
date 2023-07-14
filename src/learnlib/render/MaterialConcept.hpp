#pragma once
#include "GLObjects.hpp"

#include <concepts>
#include <glbinding/gl/gl.h>
#include <array>


namespace josh {


/*
In order to implement a material, a material class M requires:

- A typedef M::locations_type where the type aliased contains enough info
to bind all the materials to the conforming shader.

- Two overloads of a public member function M::apply(). Both should bind the textures
and set the uniforms on the conforming shader. One should take M::locations_type,
the other will query them by their names.

- Two overloads of a public member function M::query_locations() that take
either ShaderProgram or ActiveShaderProgram and return M::locations_type.

See the material concept definition for concrete info about this. This description is
largely redundant.

What's not redundant is the implicit concept of a 'conforming shader', which insofar
is not expressed in code. The shader is conforming if it implements uniforms, that
allow the material M to be applied.

The material implicitly (in comments or otherwise) declares a set of uniform names
and types that must exist in a shader, so that their locations could be queried
and the material could be applied.

For example, MaterialDS declares three uniforms:

sampler2D material.diffuse;
sampler2D material.specular;
float     material.shininess;

Which in an actual GLSL shader would be implemented as:

uniform struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
} material;

*/
template<typename M>
concept material =
    requires(const M& mat, ActiveShaderProgram& asp, const typename M::locations_type& locs) {
        { mat.apply(asp) };
        { mat.apply(asp, locs) };
    } &&
    requires(const M& mat, ActiveShaderProgram& asp, ShaderProgram& sp) {
        { mat.query_locations(asp) } -> std::same_as<typename M::locations_type>;
        { mat.query_locations(sp) } -> std::same_as<typename M::locations_type>;
    };



} // namespace josh
