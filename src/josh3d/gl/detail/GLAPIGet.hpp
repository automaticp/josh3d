#pragma once
#include "GLAPI.hpp"
#include "GLScalars.hpp"


namespace josh::glapi {


namespace detail {


inline auto get_boolean(GLenum pname) noexcept {
    GLboolean result;
    gl::glGetBooleanv(pname, &result);
    return bool(result);
}

inline auto get_boolean_indexed(GLenum pname, GLuint index) noexcept {
    GLboolean result;
    gl::glGetBooleani_v(pname, index, &result);
    return bool(result);
}

template<size_t N>
inline auto get_booleanv(GLenum pname) noexcept {
    std::array<GLboolean, N> result;
    gl::glGetBooleanv(pname, result.data());
    return result;
}

template<size_t N>
inline auto get_booleanv_indexed(GLenum pname, GLuint index) noexcept {
    std::array<GLboolean, N> result;
    gl::glGetBooleani_v(pname, index, result.data());
    return result;
}




inline auto get_integer(GLenum pname) noexcept {
    GLint result;
    gl::glGetIntegerv(pname, &result);
    return result;
}

inline auto get_integer_indexed(GLenum pname, GLuint index) noexcept {
    GLint result;
    gl::glGetIntegeri_v(pname, index, &result);
    return result;
}

template<size_t N>
inline auto get_integerv(GLenum pname) noexcept {
    std::array<GLint, N> result;
    gl::glGetIntegerv(pname, result.data());
    return result;
}

template<size_t N>
inline auto get_integerv_indexed(GLenum pname, GLuint index) noexcept {
    std::array<GLint, N> result;
    gl::glGetIntegeri_v(pname, index, result.data());
    return result;
}




template<typename EnumT = GLenum>
inline auto get_enum(GLenum pname) noexcept {
    GLenum result;
    gl::glGetIntegerv(pname, &result);
    return enum_cast<EnumT>(result);
}

template<typename EnumT = GLenum>
inline auto get_enum_indexed(GLenum pname, GLuint index) noexcept {
    GLenum result;
    gl::glGetIntegeri_v(pname, index, &result);
    return enum_cast<EnumT>(result);
}




inline auto get_integer64(GLenum pname) noexcept {
    GLint64 result;
    gl::glGetInteger64v(pname, &result);
    return result;
}

inline auto get_integer64_indexed(GLenum pname, GLuint index) noexcept {
    GLint64 result;
    gl::glGetInteger64i_v(pname, index, &result);
    return result;
}




inline auto get_float(GLenum pname) noexcept {
    GLfloat result;
    gl::glGetFloatv(pname, &result);
    return result;
}

inline auto get_float_indexed(GLenum pname, GLuint index) noexcept {
    GLfloat result;
    gl::glGetFloati_v(pname, index, &result);
    return result;
}

template<size_t N>
inline auto get_floatv(GLenum pname) noexcept {
    std::array<GLfloat, N> result;
    gl::glGetFloatv(pname, result.data());
    return result;
}

template<size_t N>
inline auto get_floatv_indexed(GLenum pname, GLuint index) noexcept {
    std::array<GLfloat, N> result;
    gl::glGetFloati_v(pname, index, result.data());
    return result;
}




} // namespace detail
namespace queries::detail {
    using josh::glapi::detail::get_boolean;
    using josh::glapi::detail::get_boolean_indexed;
    using josh::glapi::detail::get_booleanv;
    using josh::glapi::detail::get_booleanv_indexed;
    using josh::glapi::detail::get_integer;
    using josh::glapi::detail::get_integer_indexed;
    using josh::glapi::detail::get_integerv;
    using josh::glapi::detail::get_integerv_indexed;
    using josh::glapi::detail::get_enum;
    using josh::glapi::detail::get_enum_indexed;
    using josh::glapi::detail::get_integer64;
    using josh::glapi::detail::get_integer64_indexed;
    using josh::glapi::detail::get_float;
    using josh::glapi::detail::get_float_indexed;
    using josh::glapi::detail::get_floatv;
    using josh::glapi::detail::get_floatv_indexed;
} // namespace queries::detail
namespace limits::detail {
    using josh::glapi::detail::get_boolean;
    using josh::glapi::detail::get_boolean_indexed;
    using josh::glapi::detail::get_booleanv;
    using josh::glapi::detail::get_booleanv_indexed;
    using josh::glapi::detail::get_integer;
    using josh::glapi::detail::get_integer_indexed;
    using josh::glapi::detail::get_integerv;
    using josh::glapi::detail::get_integerv_indexed;
    using josh::glapi::detail::get_enum;
    using josh::glapi::detail::get_enum_indexed;
    using josh::glapi::detail::get_integer64;
    using josh::glapi::detail::get_integer64_indexed;
    using josh::glapi::detail::get_float;
    using josh::glapi::detail::get_float_indexed;
    using josh::glapi::detail::get_floatv;
    using josh::glapi::detail::get_floatv_indexed;
} // namespace queries::detail


} // namespace josh::glapi
