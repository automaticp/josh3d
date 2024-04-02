#pragma once
#include "GLAPI.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <string>
#include <string_view>
#include <span>




namespace josh {


enum class ShaderTarget : GLuint {
    ComputeShader        = GLuint(gl::GL_COMPUTE_SHADER),
    VertexShader         = GLuint(gl::GL_VERTEX_SHADER),
    TessControlShader    = GLuint(gl::GL_TESS_CONTROL_SHADER),
    TessEvaluationShader = GLuint(gl::GL_TESS_EVALUATION_SHADER),
    GeometryShader       = GLuint(gl::GL_GEOMETRY_SHADER),
    FragmentShader       = GLuint(gl::GL_FRAGMENT_SHADER),
};



namespace dsa {


namespace detail {


template<typename CRTP, ShaderTarget TargetV>
struct ShaderDSAInterface {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mutability = mutability_traits<CRTP>::mutability;
public:
    // Wraps `glShaderSource` with `count = 1`.
    //
    // OpenGL copies the shader source code strings when `glShaderSource` is called, so an application
    // may free its copy of the source code strings immediately after the function returns.
    //
    // Overload for a string with explicit size.
    void set_source(std::string_view source_string) const noexcept
        requires gl_mutable<mutability>
    {
        const GLchar* data = source_string.data();
        const GLint length = static_cast<GLint>(source_string.length());
        gl::glShaderSource(self_id(), 1, &data, &length);
    }

    // Wraps `glShaderSource` with `count = 1` and `length = nullptr`.
    //
    // OpenGL copies the shader source code strings when `glShaderSource` is called, so an application
    // may free its copy of the source code strings immediately after the function returns.
    //
    // Overload for a null-terminated string.
    void set_source(const GLchar* src) const noexcept
        requires gl_mutable<mutability>
    {
        gl::glShaderSource(self_id(), 1, &src, nullptr);
    }

    // Wraps `glShaderSource` with `count = source_strings.size()` and `length = lenghts.data()`.
    //
    // Sizes of `source_strings` and `lengths` must be equal.
    //
    // OpenGL copies the shader source code strings when `glShaderSource` is called, so an application
    // may free its copy of the source code strings immediately after the function returns.
    //
    // Each element in the `lengths` array may contain the length of the corresponding string
    // (the null character is not counted as part of the string length) or a value less than 0
    // to indicate that the string is null terminated.
    //
    // Overload for specifying a collection of source strings, reasonably unsafe...
    void set_sources(
        std::span<const GLchar*> source_strings,
        std::span<const GLint>   lengths) const noexcept
            requires gl_mutable<mutability>
    {
        assert(source_strings.size() == lengths.size());
        gl::glShaderSource(
            self_id(), source_strings.size(), source_strings.data(), lengths.data()
        );
    }


    // Wraps `glCompileShader`.
    void compile() const noexcept
        requires gl_mutable<mutability>
    {
        gl::glCompileShader(self_id());
    }


    // Wraps `glGetShaderiv` with `pname = GL_COMPILE_STATUS`.
    bool has_compiled_successfully() const noexcept {
        GLboolean is_success;
        gl::glGetShaderiv(self_id(), gl::GL_COMPILE_STATUS, &is_success);
        return is_success == gl::GL_TRUE;
    }

    // Wraps `glGetShaderiv` with `pname = GL_DELETE_STATUS`.
    //
    // Shader objects can be deleted with the command `glDeleteShader`.
    // If shader is not attached to any program object, it is deleted immediately.
    // Otherwise, shader is flagged for deletion and will be deleted when it is no longer
    // attached to any program object. If an object is flagged for deletion, its boolean
    // status bit `GL_DELETE_STATUS` is set to true.
    bool is_flagged_for_deletion() const noexcept {
        GLboolean is_flagged;
        gl::glGetShaderiv(self_id(), gl::GL_DELETE_STATUS, &is_flagged);
        return is_flagged == gl::GL_TRUE;
    }

    // Wraps `glGetShaderInfoLog`.
    //
    // The information log for a shader object is a string that may contain
    // diagnostic messages, warning messages, and other information about the last compile operation.
    // When a shader object is created, its information log will be a string of length 0.
    auto get_info_log() const noexcept
        -> std::string
    {
        GLint length_with_null_terminator;
        gl::glGetShaderiv(self_id(), gl::GL_INFO_LOG_LENGTH, &length_with_null_terminator);
        if (length_with_null_terminator <= 0) {
            return {};
        }
        std::string log;
        log.resize(static_cast<std::string::size_type>(length_with_null_terminator - 1));
        gl::glGetShaderInfoLog(
            self_id(), length_with_null_terminator, nullptr, log.data()
        );
        return log;
    }

    // Wraps `glGetShaderSource`.
    //
    // Returns the concatenation of the source code strings
    // provided in a previous call to `set_source` (aka `glShaderSource`).
    auto get_source() const noexcept
        -> std::string
    {
        GLint length_with_null_terminator;
        gl::glGetShaderiv(self_id(), gl::GL_SHADER_SOURCE_LENGTH, &length_with_null_terminator);
        if (length_with_null_terminator <= 0) {
            return {};
        }
        std::string source;
        source.resize(static_cast<std::string::size_type>(length_with_null_terminator - 1));
        gl::glGetShaderSource(
            self_id(), length_with_null_terminator, nullptr, source.data()
        );
        return source;
    }
};



} // namespace detail



namespace detail {
// TODO: Remove later.
using josh::detail::RawGLHandle;
} // namespace detail


#define JOSH3D_GENERATE_DSA_SHADER_CLASSES(Name)                                                    \
template<mutability_tag MutT = GLMutable>                                                           \
class Raw##Name                                                                                     \
    : public detail::RawGLHandle<MutT>                                                              \
    , public detail::ShaderDSAInterface<Raw##Name<MutT>, ShaderTarget::Name>                        \
{                                                                                                   \
public:                                                                                             \
    static constexpr GLKind       kind_type   = GLKind::Shader;                                     \
    static constexpr ShaderTarget target_type = ShaderTarget::Name;                                 \
    JOSH3D_MAGIC_CONSTRUCTORS_2(Raw##Name, mutability_traits<Raw##Name>, detail::RawGLHandle<MutT>) \
};                                                                                                  \
static_assert(sizeof(Raw##Name<GLMutable>) == sizeof(Raw##Name<GLConst>));


JOSH3D_GENERATE_DSA_SHADER_CLASSES(ComputeShader)
JOSH3D_GENERATE_DSA_SHADER_CLASSES(VertexShader)
JOSH3D_GENERATE_DSA_SHADER_CLASSES(TessControlShader)
JOSH3D_GENERATE_DSA_SHADER_CLASSES(TessEvaluationShader)
JOSH3D_GENERATE_DSA_SHADER_CLASSES(GeometryShader)
JOSH3D_GENERATE_DSA_SHADER_CLASSES(FragmentShader)

#undef JOSH3D_GENERATE_DSA_SHADER_CLASSES



} // namespace dsa
} // namespace josh
