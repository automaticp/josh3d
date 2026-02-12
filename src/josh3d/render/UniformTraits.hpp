#pragma once
// IWYU pragma: always_keep
#include "GLUniformTraits.hpp"
#include "GLProgram.hpp"
#include "Math.hpp"


namespace josh {


JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(vec1,   program.set_uniform_float  (loc, v.x)                   );
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(vec2,   program.set_uniform_vec2v  (loc, 1, value_ptr(v))       );
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(vec3,   program.set_uniform_vec3v  (loc, 1, value_ptr(v))       );
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(vec4,   program.set_uniform_vec4v  (loc, 1, value_ptr(v))       );
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(quat,   program.set_uniform_vec4v  (loc, 1, value_ptr(v))       );
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(mat2,   program.set_uniform_mat2v  (loc, 1, false, value_ptr(v)));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(mat2x3, program.set_uniform_mat2x3v(loc, 1, false, value_ptr(v)));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(mat2x4, program.set_uniform_mat2x4v(loc, 1, false, value_ptr(v)));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(mat3x2, program.set_uniform_mat3x2v(loc, 1, false, value_ptr(v)));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(mat3,   program.set_uniform_mat3v  (loc, 1, false, value_ptr(v)));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(mat3x4, program.set_uniform_mat3x4v(loc, 1, false, value_ptr(v)));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(mat4x2, program.set_uniform_mat4x2v(loc, 1, false, value_ptr(v)));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(mat4x3, program.set_uniform_mat4x3v(loc, 1, false, value_ptr(v)));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(mat4,   program.set_uniform_mat4v  (loc, 1, false, value_ptr(v)));


} // namespace josh

