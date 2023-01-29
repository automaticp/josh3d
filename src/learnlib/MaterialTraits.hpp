#pragma once
#include "GLObjects.hpp"

#include <concepts>
#include <glbinding/gl/gl.h>
#include <array>


namespace learn {


// There are no traits here, I killed them.








// Here's a concept instead.

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



} // namespace learn
