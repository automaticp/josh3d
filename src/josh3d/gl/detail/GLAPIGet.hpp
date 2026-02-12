#pragma once
#include "GLAPI.hpp"
#include "GLScalars.hpp"


namespace josh::glapi::detail {


inline auto get_boolean(GLenum pname)
{
    GLboolean result;
    gl::glGetBooleanv(pname, &result);
    return bool(result);
}

inline auto get_boolean_indexed(GLenum pname, GLuint index)
{
    GLboolean result;
    gl::glGetBooleani_v(pname, index, &result);
    return bool(result);
}

template<size_t N>
inline auto get_booleanv(GLenum pname)
{
    std::array<GLboolean, N> result;
    gl::glGetBooleanv(pname, result.data());
    return result;
}

template<size_t N>
inline auto get_booleanv_indexed(GLenum pname, GLuint index)
{
    std::array<GLboolean, N> result;
    gl::glGetBooleani_v(pname, index, result.data());
    return result;
}

inline auto get_integer(GLenum pname)
{
    GLint result;
    gl::glGetIntegerv(pname, &result);
    return result;
}

inline auto get_integer_indexed(GLenum pname, GLuint index)
{
    GLint result;
    gl::glGetIntegeri_v(pname, index, &result);
    return result;
}

template<size_t N>
inline auto get_integerv(GLenum pname)
{
    std::array<GLint, N> result;
    gl::glGetIntegerv(pname, result.data());
    return result;
}

template<size_t N>
inline auto get_integerv_indexed(GLenum pname, GLuint index)
{
    std::array<GLint, N> result;
    gl::glGetIntegeri_v(pname, index, result.data());
    return result;
}

template<typename EnumT = GLenum>
inline auto get_enum(GLenum pname)
{
    GLenum result;
    gl::glGetIntegerv(pname, &result);
    return enum_cast<EnumT>(result);
}

template<typename EnumT = GLenum>
inline auto get_enum_indexed(GLenum pname, GLuint index)
{
    GLenum result;
    gl::glGetIntegeri_v(pname, index, &result);
    return enum_cast<EnumT>(result);
}

inline auto get_integer64(GLenum pname)
{
    GLint64 result;
    gl::glGetInteger64v(pname, &result);
    return result;
}

inline auto get_integer64_indexed(GLenum pname, GLuint index)
{
    GLint64 result;
    gl::glGetInteger64i_v(pname, index, &result);
    return result;
}

inline auto get_float(GLenum pname)
{
    GLfloat result;
    gl::glGetFloatv(pname, &result);
    return result;
}

inline auto get_float_indexed(GLenum pname, GLuint index)
{
    GLfloat result;
    gl::glGetFloati_v(pname, index, &result);
    return result;
}

template<size_t N>
inline auto get_floatv(GLenum pname)
{
    std::array<GLfloat, N> result;
    gl::glGetFloatv(pname, result.data());
    return result;
}

template<size_t N>
inline auto get_floatv_indexed(GLenum pname, GLuint index)
{
    std::array<GLfloat, N> result;
    gl::glGetFloati_v(pname, index, result.data());
    return result;
}


} // namespace josh::glapi::detail
