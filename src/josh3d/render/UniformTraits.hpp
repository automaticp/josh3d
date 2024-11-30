#pragma once
// IWYU pragma: always_keep
#include "SingleArgMacro.hpp"
#include "GLUniformTraits.hpp"
#include "GLProgram.hpp"
#include "Math.hpp"


namespace josh {


#define JOSH3D_SPECIALIZE_UNIFORM_TRAITS(UniformT, Body)                                       \
    template<> struct uniform_traits<UniformT> {                                               \
        static void set(RawProgram<> program, Location location, const UniformT& v) noexcept { \
            JOSH3D_SINGLE_ARG(Body)                                                            \
        }                                                                                      \
    };


JOSH3D_SPECIALIZE_UNIFORM_TRAITS(vec1, program.set_uniform_float(location, v.x);)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(vec2, program.set_uniform_vec2v(location, 1, value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(vec3, program.set_uniform_vec3v(location, 1, value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(vec4, program.set_uniform_vec4v(location, 1, value_ptr(v));)

JOSH3D_SPECIALIZE_UNIFORM_TRAITS(quat, program.set_uniform_vec4v(location, 1, value_ptr(v));)

JOSH3D_SPECIALIZE_UNIFORM_TRAITS(mat2,   program.set_uniform_mat2v  (location, 1, false, value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(mat2x3, program.set_uniform_mat2x3v(location, 1, false, value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(mat2x4, program.set_uniform_mat2x4v(location, 1, false, value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(mat3x2, program.set_uniform_mat3x2v(location, 1, false, value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(mat3,   program.set_uniform_mat3v  (location, 1, false, value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(mat3x4, program.set_uniform_mat3x4v(location, 1, false, value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(mat4x2, program.set_uniform_mat4x2v(location, 1, false, value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(mat4x3, program.set_uniform_mat4x3v(location, 1, false, value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(mat4,   program.set_uniform_mat4v  (location, 1, false, value_ptr(v));)


#undef JOSH3D_SPECIALIZE_UNIFORM_TRAITS


} // namespace josh

