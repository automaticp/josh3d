#pragma once
#include "EnumUtils.hpp"
#include "GLAPI.hpp"
#include "GLAPIBinding.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "DecayToRaw.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include <glbinding/gl/boolean.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include "GLUniformTraits.hpp"
#include <string>
#include <type_traits>



namespace josh {


enum class ProgramResource : GLuint
{
    Uniform                         = GLuint(gl::GL_UNIFORM),
    Input                           = GLuint(gl::GL_PROGRAM_INPUT),
    Output                          = GLuint(gl::GL_PROGRAM_OUTPUT),
    VertexSubroutineUniform         = GLuint(gl::GL_VERTEX_SUBROUTINE_UNIFORM),
    TessControlSubroutineUniform    = GLuint(gl::GL_TESS_CONTROL_SUBROUTINE_UNIFORM),
    TessEvaluationSubroutineUniform = GLuint(gl::GL_TESS_EVALUATION_SUBROUTINE_UNIFORM),
    GeometrySubroutineUniform       = GLuint(gl::GL_GEOMETRY_SUBROUTINE_UNIFORM),
    FragmentSubroutineUniform       = GLuint(gl::GL_FRAGMENT_SUBROUTINE_UNIFORM),
    ComputeSubroutineUniform        = GLuint(gl::GL_COMPUTE_SUBROUTINE_UNIFORM),
};


namespace detail {


template<typename CRTP>
struct ProgramDSAInterface_ResourceQueries {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mutability = mutability_traits<CRTP>::mutability;
public:
    // Wraps `glGetUniformLocation`.
    //
    // Is equivalent to calling `get_resource_location(ProgramResource::Uniform, name)`.
    auto get_uniform_location(const GLchar* name) const noexcept
        -> Location
    {
        return Location{ gl::glGetUniformLocation(self_id(), name) };
    }

    // Wraps `glGetProgramResourceLocation` with `programInterface = resource`.
    auto get_resource_location(ProgramResource resource, const GLchar* name) const noexcept
        -> Location
    {
        return Location{ gl::glGetProgramResourceLocation(self_id(), enum_cast<GLenum>(resource), name) };
    }

    // Wraps `glGetProgramResourceLocationIndex` with `pname = GL_PROGRAM_OUTPUT`.
    //
    // Returns a single integer identifying the fragment color index
    // of an active fragment shader output variable.
    auto get_fragment_output_index(const GLchar* name) const noexcept
        -> Location
    {
        return Location{ gl::glGetProgramResourceLocationIndex(self_id(), gl::GL_PROGRAM_OUTPUT, name) };
    }


};




template<typename CRTP>
struct ProgramDSAInterface_Uniforms
    : ProgramDSAInterface_ResourceQueries<CRTP>
{
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mutability = mutability_traits<CRTP>::mutability;
public:

    // Wraps `glProgramUniform*` with the arguments deduced based on the custom specialization of `uniform_traits`.
    template<typename ...Args>
        requires specialized_uniform_traits_set<std::remove_cvref_t<Args>...>
    void uniform(Location location, Args&&... args) const noexcept
        requires gl_mutable<mutability>
    {
        uniform_traits<std::remove_cvref_t<Args>...>::set(static_cast<const CRTP&>(*this), location, std::forward<Args>(args)...);
    }

    // Wraps `glProgramUniform*` with the arguments deduced based on the custom specialization of `uniform_traits`.
    //
    // Equivalent to `uniform(program.get_uniform_location(name), args...)`.
    template<typename ...Args>
        requires specialized_uniform_traits_set<std::remove_cvref_t<Args>...>
    void uniform(const GLchar* name, Args&&... args) const noexcept
        requires gl_mutable<mutability>
    {
        uniform(this->get_uniform_location(name), std::forward<Args>(args)...);
    }


    // Editor with good multiline editing capabilities is required :^)

    //
    void set_uniform_float              (Location location, GLfloat  x) const noexcept requires gl_mutable<mutability>                                                { gl::glProgramUniform1f             (self_id(), location, x);                       }
    void set_uniform_double             (Location location, GLdouble x) const noexcept requires gl_mutable<mutability>                                                { gl::glProgramUniform1d             (self_id(), location, x);                       }
    void set_uniform_int                (Location location, GLint    x) const noexcept requires gl_mutable<mutability>                                                { gl::glProgramUniform1i             (self_id(), location, x);                       }
    void set_uniform_int64_ARB          (Location location, GLint64  x) const noexcept requires gl_mutable<mutability>                                                { gl::glProgramUniform1i64ARB        (self_id(), location, x);                       }
    void set_uniform_uint               (Location location, GLuint   x) const noexcept requires gl_mutable<mutability>                                                { gl::glProgramUniform1ui            (self_id(), location, x);                       }
    void set_uniform_uint64_ARB         (Location location, GLuint64 x) const noexcept requires gl_mutable<mutability>                                                { gl::glProgramUniform1ui64ARB       (self_id(), location, x);                       }
    void set_uniform_vec2               (Location location, GLfloat  x, GLfloat  y) const noexcept requires gl_mutable<mutability>                                    { gl::glProgramUniform2f             (self_id(), location, x, y);                    }
    void set_uniform_dvec2              (Location location, GLdouble x, GLdouble y) const noexcept requires gl_mutable<mutability>                                    { gl::glProgramUniform2d             (self_id(), location, x, y);                    }
    void set_uniform_ivec2              (Location location, GLint    x, GLint    y) const noexcept requires gl_mutable<mutability>                                    { gl::glProgramUniform2i             (self_id(), location, x, y);                    }
    void set_uniform_i64vec2_ARB        (Location location, GLint64  x, GLint64  y) const noexcept requires gl_mutable<mutability>                                    { gl::glProgramUniform2i64ARB        (self_id(), location, x, y);                    }
    void set_uniform_uvec2              (Location location, GLuint   x, GLuint   y) const noexcept requires gl_mutable<mutability>                                    { gl::glProgramUniform2ui            (self_id(), location, x, y);                    }
    void set_uniform_u64vec2_ARB        (Location location, GLuint64 x, GLuint64 y) const noexcept requires gl_mutable<mutability>                                    { gl::glProgramUniform2ui64ARB       (self_id(), location, x, y);                    }
    void set_uniform_vec3               (Location location, GLfloat  x, GLfloat  y, GLfloat  z) const noexcept requires gl_mutable<mutability>                        { gl::glProgramUniform3f             (self_id(), location, x, y, z);                 }
    void set_uniform_dvec3              (Location location, GLdouble x, GLdouble y, GLdouble z) const noexcept requires gl_mutable<mutability>                        { gl::glProgramUniform3d             (self_id(), location, x, y, z);                 }
    void set_uniform_ivec3              (Location location, GLint    x, GLint    y, GLint    z) const noexcept requires gl_mutable<mutability>                        { gl::glProgramUniform3i             (self_id(), location, x, y, z);                 }
    void set_uniform_i64vec3_ARB        (Location location, GLint64  x, GLint64  y, GLint64  z) const noexcept requires gl_mutable<mutability>                        { gl::glProgramUniform3i64ARB        (self_id(), location, x, y, z);                 }
    void set_uniform_uvec3              (Location location, GLuint   x, GLuint   y, GLuint   z) const noexcept requires gl_mutable<mutability>                        { gl::glProgramUniform3ui            (self_id(), location, x, y, z);                 }
    void set_uniform_u64vec3_ARB        (Location location, GLuint64 x, GLuint64 y, GLuint64 z) const noexcept requires gl_mutable<mutability>                        { gl::glProgramUniform3ui64ARB       (self_id(), location, x, y, z);                 }
    void set_uniform_vec4               (Location location, GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w) const noexcept requires gl_mutable<mutability>            { gl::glProgramUniform4f             (self_id(), location, x, y, z, w);              }
    void set_uniform_dvec4              (Location location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) const noexcept requires gl_mutable<mutability>            { gl::glProgramUniform4d             (self_id(), location, x, y, z, w);              }
    void set_uniform_ivec4              (Location location, GLint    x, GLint    y, GLint    z, GLint    w) const noexcept requires gl_mutable<mutability>            { gl::glProgramUniform4i             (self_id(), location, x, y, z, w);              }
    void set_uniform_i64vec4_ARB        (Location location, GLint64  x, GLint64  y, GLint64  z, GLint64  w) const noexcept requires gl_mutable<mutability>            { gl::glProgramUniform4i64ARB        (self_id(), location, x, y, z, w);              }
    void set_uniform_uvec4              (Location location, GLuint   x, GLuint   y, GLuint   z, GLuint   w) const noexcept requires gl_mutable<mutability>            { gl::glProgramUniform4ui            (self_id(), location, x, y, z, w);              }
    void set_uniform_u64vec4_ARB        (Location location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w) const noexcept requires gl_mutable<mutability>            { gl::glProgramUniform4ui64ARB       (self_id(), location, x, y, z, w);              }
    void set_uniform_floatv             (Location location, GLsizei count, const GLfloat*  value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform1fv            (self_id(), location, count, value);            }
    void set_uniform_doublev            (Location location, GLsizei count, const GLdouble* value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform1dv            (self_id(), location, count, value);            }
    void set_uniform_intv               (Location location, GLsizei count, const GLint*    value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform1iv            (self_id(), location, count, value);            }
    void set_uniform_int64v_ARB         (Location location, GLsizei count, const GLint64*  value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform1i64vARB       (self_id(), location, count, value);            }
    void set_uniform_uintv              (Location location, GLsizei count, const GLuint*   value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform1uiv           (self_id(), location, count, value);            }
    void set_uniform_uint64v_ARB        (Location location, GLsizei count, const GLuint64* value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform1ui64vARB      (self_id(), location, count, value);            }
    void set_uniform_vec2v              (Location location, GLsizei count, const GLfloat*  value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform2fv            (self_id(), location, count, value);            }
    void set_uniform_dvec2v             (Location location, GLsizei count, const GLdouble* value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform2dv            (self_id(), location, count, value);            }
    void set_uniform_ivec2v             (Location location, GLsizei count, const GLint*    value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform2iv            (self_id(), location, count, value);            }
    void set_uniform_i64vec2v_ARB       (Location location, GLsizei count, const GLint64*  value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform2i64vARB       (self_id(), location, count, value);            }
    void set_uniform_uvec2v             (Location location, GLsizei count, const GLuint*   value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform2uiv           (self_id(), location, count, value);            }
    void set_uniform_u64vec2v_ARB       (Location location, GLsizei count, const GLuint64* value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform2ui64vARB      (self_id(), location, count, value);            }
    void set_uniform_vec3v              (Location location, GLsizei count, const GLfloat*  value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform3fv            (self_id(), location, count, value);            }
    void set_uniform_dvec3v             (Location location, GLsizei count, const GLdouble* value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform3dv            (self_id(), location, count, value);            }
    void set_uniform_ivec3v             (Location location, GLsizei count, const GLint*    value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform3iv            (self_id(), location, count, value);            }
    void set_uniform_i64vec3v_ARB       (Location location, GLsizei count, const GLint64*  value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform3i64vARB       (self_id(), location, count, value);            }
    void set_uniform_uvec3v             (Location location, GLsizei count, const GLuint*   value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform3uiv           (self_id(), location, count, value);            }
    void set_uniform_u64vec3v_ARB       (Location location, GLsizei count, const GLuint64* value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform3ui64vARB      (self_id(), location, count, value);            }
    void set_uniform_vec4v              (Location location, GLsizei count, const GLfloat*  value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform4fv            (self_id(), location, count, value);            }
    void set_uniform_dvec4v             (Location location, GLsizei count, const GLdouble* value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform4dv            (self_id(), location, count, value);            }
    void set_uniform_ivec4v             (Location location, GLsizei count, const GLint*    value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform4iv            (self_id(), location, count, value);            }
    void set_uniform_i64vec4v_ARB       (Location location, GLsizei count, const GLint64*  value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform4i64vARB       (self_id(), location, count, value);            }
    void set_uniform_uvec4v             (Location location, GLsizei count, const GLuint*   value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform4uiv           (self_id(), location, count, value);            }
    void set_uniform_u64vec4v_ARB       (Location location, GLsizei count, const GLuint64* value) const noexcept requires gl_mutable<mutability>                      { gl::glProgramUniform4ui64vARB      (self_id(), location, count, value);            }
    void set_uniform_mat2v              (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix2fv      (self_id(), location, count, transpose, value); }
    void set_uniform_mat2x3v            (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix2x3fv    (self_id(), location, count, transpose, value); }
    void set_uniform_mat2x4v            (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix2x4fv    (self_id(), location, count, transpose, value); }
    void set_uniform_dmat2v             (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix2dv      (self_id(), location, count, transpose, value); }
    void set_uniform_dmat2x3v           (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix2x3dv    (self_id(), location, count, transpose, value); }
    void set_uniform_dmat2x4v           (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix2x4dv    (self_id(), location, count, transpose, value); }
    void set_uniform_mat3v              (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix3fv      (self_id(), location, count, transpose, value); }
    void set_uniform_mat3x2v            (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix3x2fv    (self_id(), location, count, transpose, value); }
    void set_uniform_mat3x4v            (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix3x4fv    (self_id(), location, count, transpose, value); }
    void set_uniform_dmat3v             (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix3dv      (self_id(), location, count, transpose, value); }
    void set_uniform_dmat3x2v           (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix3x2dv    (self_id(), location, count, transpose, value); }
    void set_uniform_dmat3x4v           (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix3x4dv    (self_id(), location, count, transpose, value); }
    void set_uniform_mat4v              (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix4fv      (self_id(), location, count, transpose, value); }
    void set_uniform_mat4x2v            (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix4x2fv    (self_id(), location, count, transpose, value); }
    void set_uniform_mat4x3v            (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix4x3fv    (self_id(), location, count, transpose, value); }
    void set_uniform_dmat4v             (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix4dv      (self_id(), location, count, transpose, value); }
    void set_uniform_dmat4x2v           (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix4x2dv    (self_id(), location, count, transpose, value); }
    void set_uniform_dmat4x3v           (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept requires gl_mutable<mutability> { gl::glProgramUniformMatrix4x3dv    (self_id(), location, count, transpose, value); }
    void set_uniform_handle_uint64_ARB  (Location location, GLuint64 value) const noexcept requires gl_mutable<mutability>                                            { gl::glProgramUniformHandleui64ARB  (self_id(), location, value);                   }
    void set_uniform_handle_uint64v_ARB (Location location, GLsizei count, const GLuint64* values) const noexcept requires gl_mutable<mutability>                     { gl::glProgramUniformHandleui64vARB (self_id(), location, count, values);           }


    // Redundant extensions, most likely no point in using this:

    // void set_uniform_1dEXT            (Location location, GLdouble x) const noexcept                                                                  { gl::glProgramUniform1dEXT     (self_id(), location, x);              }
    // void set_uniform_1fEXT            (Location location, GLfloat v0) const noexcept                                                                  { gl::glProgramUniform1fEXT     (self_id(), location, v0);             }
    // void set_uniform_1iEXT            (Location location, GLint v0) const noexcept                                                                    { gl::glProgramUniform1iEXT     (self_id(), location, v0);             }
    // void set_uniform_1i64NV           (Location location, gl::GLint64EXT x) const noexcept                                                            { gl::glProgramUniform1i64NV    (self_id(), location, x);              }
    // void set_uniform_1ui64NV          (Location location, gl::GLuint64EXT x) const noexcept                                                           { gl::glProgramUniform1ui64NV   (self_id(), location, x);              }
    // void set_uniform_1uiEXT           (Location location, GLuint v0) const noexcept                                                                   { gl::glProgramUniform1uiEXT    (self_id(), location, v0);             }
    // void set_uniform_2dEXT            (Location location, GLdouble x, GLdouble y) const noexcept                                                      { gl::glProgramUniform2dEXT     (self_id(), location, x, y);           }
    // void set_uniform_2fEXT            (Location location, GLfloat v0, GLfloat v1) const noexcept                                                      { gl::glProgramUniform2fEXT     (self_id(), location, v0, v1);         }
    // void set_uniform_2i64NV           (Location location, gl::GLint64EXT x, gl::GLint64EXT y) const noexcept                                          { gl::glProgramUniform2i64NV    (self_id(), location, x, y);           }
    // void set_uniform_2iEXT            (Location location, GLint v0, GLint v1) const noexcept                                                          { gl::glProgramUniform2iEXT     (self_id(), location, v0, v1);         }
    // void set_uniform_2ui64NV          (Location location, gl::GLuint64EXT x, gl::GLuint64EXT y) const noexcept                                        { gl::glProgramUniform2ui64NV   (self_id(), location, x, y);           }
    // void set_uniform_2uiEXT           (Location location, GLuint v0, GLuint v1) const noexcept                                                        { gl::glProgramUniform2uiEXT    (self_id(), location, v0, v1);         }
    // void set_uniform_3dEXT            (Location location, GLdouble x, GLdouble y, GLdouble z) const noexcept                                          { gl::glProgramUniform3dEXT     (self_id(), location, x, y, z);        }
    // void set_uniform_3fEXT            (Location location, GLfloat v0, GLfloat v1, GLfloat v2) const noexcept                                          { gl::glProgramUniform3fEXT     (self_id(), location, v0, v1, v2);     }
    // void set_uniform_3iEXT            (Location location, GLint v0, GLint v1, GLint v2) const noexcept                                                { gl::glProgramUniform3iEXT     (self_id(), location, v0, v1, v2);     }
    // void set_uniform_3i64NV           (Location location, gl::GLint64EXT x, gl::GLint64EXT y, gl::GLint64EXT z) const noexcept                        { gl::glProgramUniform3i64NV    (self_id(), location, x, y, z);        }
    // void set_uniform_3ui64NV          (Location location, gl::GLuint64EXT x, gl::GLuint64EXT y, gl::GLuint64EXT z) const noexcept                     { gl::glProgramUniform3ui64NV   (self_id(), location, x, y, z);        }
    // void set_uniform_3uiEXT           (Location location, GLuint v0, GLuint v1, GLuint v2) const noexcept                                             { gl::glProgramUniform3uiEXT    (self_id(), location, v0, v1, v2);     }
    // void set_uniform_4dEXT            (Location location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) const noexcept                              { gl::glProgramUniform4dEXT     (self_id(), location, x, y, z, w);     }
    // void set_uniform_4fEXT            (Location location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const noexcept                              { gl::glProgramUniform4fEXT     (self_id(), location, v0, v1, v2, v3); }
    // void set_uniform_4i64NV           (Location location, gl::GLint64EXT x, gl::GLint64EXT y, gl::GLint64EXT z, gl::GLint64EXT w) const noexcept      { gl::glProgramUniform4i64NV    (self_id(), location, x, y, z, w);     }
    // void set_uniform_4iEXT            (Location location, GLint v0, GLint v1, GLint v2, GLint v3) const noexcept                                      { gl::glProgramUniform4iEXT     (self_id(), location, v0, v1, v2, v3); }
    // void set_uniform_4ui64NV          (Location location, gl::GLuint64EXT x, gl::GLuint64EXT y, gl::GLuint64EXT z, gl::GLuint64EXT w) const noexcept  { gl::glProgramUniform4ui64NV   (self_id(), location, x, y, z, w);     }
    // void set_uniform_4uiEXT           (Location location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) const noexcept                                  { gl::glProgramUniform4uiEXT    (self_id(), location, v0, v1, v2, v3); }

    // void set_uniform_1i64vNV          (Location location, GLsizei count, const gl::GLint64EXT*  value) const noexcept                    { gl::glProgramUniform1i64vNV   (self_id(), location, count, value); }
    // void set_uniform_1ui64vNV         (Location location, GLsizei count, const gl::GLuint64EXT* value) const noexcept                    { gl::glProgramUniform1ui64vNV  (self_id(), location, count, value); }
    // void set_uniform_2i64vNV          (Location location, GLsizei count, const gl::GLint64EXT*  value) const noexcept                    { gl::glProgramUniform2i64vNV   (self_id(), location, count, value); }
    // void set_uniform_2ui64vNV         (Location location, GLsizei count, const gl::GLuint64EXT* value) const noexcept                    { gl::glProgramUniform2ui64vNV  (self_id(), location, count, value); }
    // void set_uniform_3i64vNV          (Location location, GLsizei count, const gl::GLint64EXT*  value) const noexcept                    { gl::glProgramUniform3i64vNV   (self_id(), location, count, value); }
    // void set_uniform_3ui64vNV         (Location location, GLsizei count, const gl::GLuint64EXT* value) const noexcept                    { gl::glProgramUniform3ui64vNV  (self_id(), location, count, value); }
    // void set_uniform_4i64vNV          (Location location, GLsizei count, const gl::GLint64EXT*  value) const noexcept                    { gl::glProgramUniform4i64vNV   (self_id(), location, count, value); }
    // void set_uniform_4ui64vNV         (Location location, GLsizei count, const gl::GLuint64EXT* value) const noexcept                    { gl::glProgramUniform4ui64vNV  (self_id(), location, count, value); }

    // void set_uniform_1fvEXT           (Location location, GLsizei count, const GLfloat*         value) const noexcept                    { gl::glProgramUniform1fvEXT    (self_id(), location, count, value); }
    // void set_uniform_1dvEXT           (Location location, GLsizei count, const GLdouble*        value) const noexcept                    { gl::glProgramUniform1dvEXT    (self_id(), location, count, value); }
    // void set_uniform_1ivEXT           (Location location, GLsizei count, const GLint*           value) const noexcept                    { gl::glProgramUniform1ivEXT    (self_id(), location, count, value); }
    // void set_uniform_1uivEXT          (Location location, GLsizei count, const GLuint*          value) const noexcept                    { gl::glProgramUniform1uivEXT   (self_id(), location, count, value); }
    // void set_uniform_2fvEXT           (Location location, GLsizei count, const GLfloat*         value) const noexcept                    { gl::glProgramUniform2fvEXT    (self_id(), location, count, value); }
    // void set_uniform_2dvEXT           (Location location, GLsizei count, const GLdouble*        value) const noexcept                    { gl::glProgramUniform2dvEXT    (self_id(), location, count, value); }
    // void set_uniform_2ivEXT           (Location location, GLsizei count, const GLint*           value) const noexcept                    { gl::glProgramUniform2ivEXT    (self_id(), location, count, value); }
    // void set_uniform_2uivEXT          (Location location, GLsizei count, const GLuint*          value) const noexcept                    { gl::glProgramUniform2uivEXT   (self_id(), location, count, value); }
    // void set_uniform_3fvEXT           (Location location, GLsizei count, const GLfloat*         value) const noexcept                    { gl::glProgramUniform3fvEXT    (self_id(), location, count, value); }
    // void set_uniform_3dvEXT           (Location location, GLsizei count, const GLdouble*        value) const noexcept                    { gl::glProgramUniform3dvEXT    (self_id(), location, count, value); }
    // void set_uniform_3ivEXT           (Location location, GLsizei count, const GLint*           value) const noexcept                    { gl::glProgramUniform3ivEXT    (self_id(), location, count, value); }
    // void set_uniform_3uivEXT          (Location location, GLsizei count, const GLuint*          value) const noexcept                    { gl::glProgramUniform3uivEXT   (self_id(), location, count, value); }
    // void set_uniform_4fvEXT           (Location location, GLsizei count, const GLfloat*         value) const noexcept                    { gl::glProgramUniform4fvEXT    (self_id(), location, count, value); }
    // void set_uniform_4dvEXT           (Location location, GLsizei count, const GLdouble*        value) const noexcept                    { gl::glProgramUniform4dvEXT    (self_id(), location, count, value); }
    // void set_uniform_4ivEXT           (Location location, GLsizei count, const GLint*           value) const noexcept                    { gl::glProgramUniform4ivEXT    (self_id(), location, count, value); }
    // void set_uniform_4uivEXT          (Location location, GLsizei count, const GLuint*          value) const noexcept                    { gl::glProgramUniform4uivEXT   (self_id(), location, count, value); }

    // void set_uniform_mat2dvEXT        (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept     { gl::glProgramUniformMatrix2dvEXT   (self_id(), location, count, transpose, value); }
    // void set_uniform_mat2fvEXT        (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept     { gl::glProgramUniformMatrix2fvEXT   (self_id(), location, count, transpose, value); }
    // void set_uniform_mat2x3dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept     { gl::glProgramUniformMatrix2x3dvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat2x3fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept     { gl::glProgramUniformMatrix2x3fvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat2x4dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept     { gl::glProgramUniformMatrix2x4dvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat2x4fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept     { gl::glProgramUniformMatrix2x4fvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat3dvEXT        (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept     { gl::glProgramUniformMatrix3dvEXT   (self_id(), location, count, transpose, value); }
    // void set_uniform_mat3fvEXT        (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept     { gl::glProgramUniformMatrix3fvEXT   (self_id(), location, count, transpose, value); }
    // void set_uniform_mat3x2dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept     { gl::glProgramUniformMatrix3x2dvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat3x2fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept     { gl::glProgramUniformMatrix3x2fvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat3x4dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept     { gl::glProgramUniformMatrix3x4dvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat3x4fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept     { gl::glProgramUniformMatrix3x4fvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat4dvEXT        (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept     { gl::glProgramUniformMatrix4dvEXT   (self_id(), location, count, transpose, value); }
    // void set_uniform_mat4fvEXT        (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept     { gl::glProgramUniformMatrix4fvEXT   (self_id(), location, count, transpose, value); }
    // void set_uniform_mat4x2dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept     { gl::glProgramUniformMatrix4x2dvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat4x2fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept     { gl::glProgramUniformMatrix4x2fvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat4x3dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const noexcept     { gl::glProgramUniformMatrix4x3dvEXT (self_id(), location, count, transpose, value); }
    // void set_uniform_mat4x3fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const noexcept     { gl::glProgramUniformMatrix4x3fvEXT (self_id(), location, count, transpose, value); }

    // void set_uniform_handleui64NV     (Location location, GLuint64 value) const noexcept                                                { gl::glProgramUniformHandleui64NV   (self_id(), location, value); }
    // void set_uniform_handleui64vNV    (Location location, GLsizei count, const GLuint64* values) const noexcept                         { gl::glProgramUniformHandleui64vNV  (self_id(), location, count, values); }


};






template<typename CRTP>
struct ProgramDSAInterface_Use {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:
    // Wraps `glUseProgram`.
    [[nodiscard("BindTokens have to be provided to an API call that expects bound state.")]]
    auto use() const
        -> BindToken<Binding::Program>
    {
        return glapi::bind_to_context<Binding::Program>(self_id());
    }
};










template<typename CRTP>
struct ProgramDSAInterface
    : ProgramDSAInterface_Uniforms<CRTP>
    , ProgramDSAInterface_Use<CRTP>
{
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
    using mutability = mutability_traits<CRTP>::mutability;
public:

    // Wraps `glAttachShader`.
    template<of_kind<GLKind::Shader> ShaderT>
    void attach_shader(const ShaderT& shader) const noexcept
        requires mt::is_mutable
    {
        gl::glAttachShader(self_id(), decay_to_raw(shader).id());
    }

    // Wraps `glDetachShader`.
    template<of_kind<GLKind::Shader> ShaderT>
    void detach_shader(const ShaderT& shader) const noexcept
        requires mt::is_mutable
    {
        gl::glDetachShader(self_id(), decay_to_raw(shader).id());
    }

    // Wraps `glLinkProgram`.
    void link() const noexcept
        requires mt::is_mutable
    {
        gl::glLinkProgram(self_id());
    }

    // Wraps `glGetProgramiv` with `pname = GL_LINK_STATUS`.
    bool has_linked_successfully() const noexcept {
        GLboolean is_success;
        gl::glGetProgramiv(self_id(), gl::GL_LINK_STATUS, &is_success);
        return is_success == gl::GL_TRUE;
    }

    // Wraps `glValidateProgram` followed by `glGetProgramiv` with `pname = GL_VALIDATE_STATUS`.
    // Returns `true` if the program is valid according to `glValidateProgram`, `false` otherwise.
    bool validate() const noexcept {
        gl::glValidateProgram(self_id());
        GLboolean is_valid;
        gl::glGetProgramiv(self_id(), gl::GL_VALIDATE_STATUS, &is_valid);
        return is_valid == gl::GL_TRUE;
    }

    //  Wraps `glGetProgramInfoLog`.
    auto get_info_log() const noexcept
        -> std::string
    {
        GLint length_with_null_terminator;
        gl::glGetProgramiv(self_id(), gl::GL_INFO_LOG_LENGTH, &length_with_null_terminator);
        if (length_with_null_terminator <= 0) {
            return {};
        }
        std::string log;
        log.resize(static_cast<std::string::size_type>(length_with_null_terminator - 1));
        gl::glGetProgramInfoLog(
            self_id(), length_with_null_terminator, nullptr, log.data()
        );
        return log;
    }

    // Wraps `glGetProgramiv` with `pname = GL_ATTACHED_SHADERS`.
    auto get_num_attached_shaders() const noexcept
        -> GLuint
    {
        GLint num_shaders;
        gl::glGetProgramiv(self_id(), gl::GL_ATTACHED_SHADERS, &num_shaders);
        return static_cast<GLuint>(num_shaders);
    }

    // Wraps `glGetProgramiv` with `pname = GL_DELETE_STATUS`.
    bool is_flagged_for_deletion() const noexcept {
        GLboolean is_flagged;
        gl::glGetProgramiv(self_id(), gl::GL_DELETE_STATUS, &is_flagged);
        return is_flagged == gl::GL_TRUE;
    }
};


} // namespace detail




template<mutability_tag MutT = GLMutable>
class RawProgram
    : public detail::RawGLHandle<MutT>
    , public detail::ProgramDSAInterface<RawProgram<MutT>>
{
public:
    static constexpr GLKind kind_type = GLKind::Program;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawProgram, mutability_traits<RawProgram>, detail::RawGLHandle<MutT>)
};
static_assert(sizeof(RawProgram<GLMutable>) == sizeof(RawProgram<GLConst>));




template<> struct uniform_traits<bool>      { static void set(RawProgram<> program, Location location, bool v)     noexcept { program.set_uniform_int   (location, v);    } };
template<> struct uniform_traits<GLint>     { static void set(RawProgram<> program, Location location, GLint v)    noexcept { program.set_uniform_int   (location, v);    } };
template<> struct uniform_traits<GLuint>    { static void set(RawProgram<> program, Location location, GLuint v)   noexcept { program.set_uniform_uint  (location, v);    } };
template<> struct uniform_traits<GLfloat>   { static void set(RawProgram<> program, Location location, GLfloat v)  noexcept { program.set_uniform_float (location, v);    } };
template<> struct uniform_traits<GLdouble>  { static void set(RawProgram<> program, Location location, GLdouble v) noexcept { program.set_uniform_double(location, v);    } };


} // namespace josh
