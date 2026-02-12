#pragma once
#include "detail/StrongScalar.hpp"
#include "GLMutability.hpp"
#include "GLScalars.hpp"


namespace josh {


JOSH3D_DEFINE_STRONG_SCALAR(Location, GLint);

template<mutability_tag MutT>
class RawProgram;


template<typename ...Args>
struct uniform_traits; // { static void set(RawProgram<GLMutable> program, Location location, const Args&... args) noexcept; }

template<typename ...Args>
concept specialized_uniform_traits_set = requires(
    RawProgram<GLMutable> program,
    Location              location,
    Args&&...             args)
{
    uniform_traits<Args...>::set(program, location, args...);
};

/*
Only works for 1-argument uniforms. Should cover about 97% of cases.
*/
#define JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(Type, ...) \
    template<> struct uniform_traits<Type>           \
    { static void set(RawProgram<> program, Location loc, const Type& v) { __VA_ARGS__; } }

/*
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(GLint,    program.set_uniform_int   (loc, v));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(GLuint,   program.set_uniform_uint  (loc, v));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(GLfloat,  program.set_uniform_float (loc, v));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(GLdouble, program.set_uniform_double(loc, v));
*/


} // namespace josh
