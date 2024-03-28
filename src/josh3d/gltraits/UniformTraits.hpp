#pragma once
#include "GLUniformTraits.hpp"
#include "GLDSAProgram.hpp"
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace josh {


#define JOSH3D_SINGLE_ARG(...) __VA_ARGS__

#define JOSH3D_SPECIALIZE_UNIFORM_TRAITS(UniformT, Body)                                            \
    template<> struct uniform_traits<UniformT> {                                                    \
        static void set(dsa::RawProgram<> program, Location location, const UniformT& v) noexcept { \
            JOSH3D_SINGLE_ARG(Body)                                                                 \
        }                                                                                           \
    };


JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::vec1, program.set_uniform_float(location, v.x);)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::vec2, program.set_uniform_vec2v(location, 1, glm::value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::vec3, program.set_uniform_vec3v(location, 1, glm::value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::vec4, program.set_uniform_vec4v(location, 1, glm::value_ptr(v));)

JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::quat, program.set_uniform_vec4v(location, 1, glm::value_ptr(v));)

JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::mat2,   program.set_uniform_mat2v  (location, 1, false, glm::value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::mat2x3, program.set_uniform_mat2x3v(location, 1, false, glm::value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::mat2x4, program.set_uniform_mat2x4v(location, 1, false, glm::value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::mat3x2, program.set_uniform_mat3x2v(location, 1, false, glm::value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::mat3,   program.set_uniform_mat3v  (location, 1, false, glm::value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::mat3x4, program.set_uniform_mat3x4v(location, 1, false, glm::value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::mat4x2, program.set_uniform_mat4x2v(location, 1, false, glm::value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::mat4x3, program.set_uniform_mat4x3v(location, 1, false, glm::value_ptr(v));)
JOSH3D_SPECIALIZE_UNIFORM_TRAITS(glm::mat4,   program.set_uniform_mat4v  (location, 1, false, glm::value_ptr(v));)


#undef JOSH3D_SPECIALIZE_UNIFORM_TRAITS


} // namespace josh

