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
concept specialized_uniform_traits_set =
    requires(RawProgram<GLMutable> program, Location location, Args&&... args) {
        uniform_traits<Args...>::set(program, location, args...);
    };


/*
template<> struct uniform_traits<GLint>     { static void set(RawProgram<> program, Location location, GLint v)    noexcept { program.set_uniform_int   (location, v);    } };
template<> struct uniform_traits<GLuint>    { static void set(RawProgram<> program, Location location, GLuint v)   noexcept { program.set_uniform_uint  (location, v);    } };
template<> struct uniform_traits<GLfloat>   { static void set(RawProgram<> program, Location location, GLfloat v)  noexcept { program.set_uniform_float (location, v);    } };
template<> struct uniform_traits<GLdouble>  { static void set(RawProgram<> program, Location location, GLdouble v) noexcept { program.set_uniform_double(location, v);    } };
*/




} // namespace josh
