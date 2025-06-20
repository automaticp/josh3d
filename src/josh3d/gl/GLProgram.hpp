#pragma once
#include "CategoryCasts.hpp"
#include "EnumUtils.hpp"
#include "GLAPI.hpp"
#include "GLAPIBinding.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "GLUniformTraits.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include "detail/MixinHeaderMacro.hpp"
#include "DecayToRaw.hpp"
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
namespace program_api {


template<typename CRTP>
struct ResourceQueries
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glGetUniformLocation`.
    //
    // Is equivalent to calling `get_resource_location(ProgramResource::Uniform, name)`.
    auto get_uniform_location(const GLchar* name) const
        -> Location
    {
        return Location{ gl::glGetUniformLocation(self_id(), name) };
    }

    // Wraps `glGetProgramResourceLocation` with `programInterface = resource`.
    auto get_resource_location(ProgramResource resource, const GLchar* name) const
        -> Location
    {
        return Location{ gl::glGetProgramResourceLocation(self_id(), enum_cast<GLenum>(resource), name) };
    }

    // Wraps `glGetProgramResourceLocationIndex` with `pname = GL_PROGRAM_OUTPUT`.
    //
    // Returns a single integer identifying the fragment color index
    // of an active fragment shader output variable.
    auto get_fragment_output_index(const GLchar* name) const
        -> Location
    {
        return Location{ gl::glGetProgramResourceLocationIndex(self_id(), gl::GL_PROGRAM_OUTPUT, name) };
    }
};


template<typename CRTP>
struct Uniforms
    : ResourceQueries<CRTP>
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glProgramUniform*` with the arguments deduced based on the custom specialization of `uniform_traits`.
    template<typename ...Args> requires specialized_uniform_traits_set<std::decay_t<Args>...>
    void uniform(Location location, Args&&... args) const
        requires mt::is_mutable
    {
        uniform_traits<std::decay_t<Args>...>::set(static_cast<const CRTP&>(*this), location, FORWARD(args)...);
    }

    // Wraps `glProgramUniform*` with the arguments deduced based on the custom specialization of `uniform_traits`.
    //
    // Equivalent to `uniform(program.get_uniform_location(name), args...)`.
    template<typename ...Args> requires specialized_uniform_traits_set<std::decay_t<Args>...>
    void uniform(const GLchar* name, Args&&... args) const
        requires mt::is_mutable
    {
        uniform(this->get_uniform_location(name), std::forward<Args>(args)...);
    }

    // Editor with good multiline editing capabilities is required :^)

#define JOSH3D_SET_UNIFORM(Name, ...) \
    void set_uniform_##Name(Location location, __VA_ARGS__) const requires mt::is_mutable JOSH3D_SET_UNIFORM_RHS
#define JOSH3D_SET_UNIFORM_RHS(GLName, ...) \
    { gl::glProgramUniform##GLName(self_id(), location, __VA_ARGS__); }

//    void set_uniform_float(Location loc, GLfloat  x) const requires is_mutable {    glProgramUniform1f(id, loc,     x);                    }
    JOSH3D_SET_UNIFORM(float,              GLfloat  x                                               )(1f,             x                      );
    JOSH3D_SET_UNIFORM(double,             GLdouble x                                               )(1d,             x                      );
    JOSH3D_SET_UNIFORM(int,                GLint    x                                               )(1i,             x                      );
    JOSH3D_SET_UNIFORM(int64_ARB,          GLint64  x                                               )(1i64ARB,        x                      );
    JOSH3D_SET_UNIFORM(uint,               GLuint   x                                               )(1ui,            x                      );
    JOSH3D_SET_UNIFORM(uint64_ARB,         GLuint64 x                                               )(1ui64ARB,       x                      );
    JOSH3D_SET_UNIFORM(vec2,               GLfloat  x, GLfloat  y                                   )(2f,             x, y                   );
    JOSH3D_SET_UNIFORM(dvec2,              GLdouble x, GLdouble y                                   )(2d,             x, y                   );
    JOSH3D_SET_UNIFORM(ivec2,              GLint    x, GLint    y                                   )(2i,             x, y                   );
    JOSH3D_SET_UNIFORM(i64vec2_ARB,        GLint64  x, GLint64  y                                   )(2i64ARB,        x, y                   );
    JOSH3D_SET_UNIFORM(uvec2,              GLuint   x, GLuint   y                                   )(2ui,            x, y                   );
    JOSH3D_SET_UNIFORM(u64vec2_ARB,        GLuint64 x, GLuint64 y                                   )(2ui64ARB,       x, y                   );
    JOSH3D_SET_UNIFORM(vec3,               GLfloat  x, GLfloat  y, GLfloat  z                       )(3f,             x, y, z                );
    JOSH3D_SET_UNIFORM(dvec3,              GLdouble x, GLdouble y, GLdouble z                       )(3d,             x, y, z                );
    JOSH3D_SET_UNIFORM(ivec3,              GLint    x, GLint    y, GLint    z                       )(3i,             x, y, z                );
    JOSH3D_SET_UNIFORM(i64vec3_ARB,        GLint64  x, GLint64  y, GLint64  z                       )(3i64ARB,        x, y, z                );
    JOSH3D_SET_UNIFORM(uvec3,              GLuint   x, GLuint   y, GLuint   z                       )(3ui,            x, y, z                );
    JOSH3D_SET_UNIFORM(u64vec3_ARB,        GLuint64 x, GLuint64 y, GLuint64 z                       )(3ui64ARB,       x, y, z                );
    JOSH3D_SET_UNIFORM(vec4,               GLfloat  x, GLfloat  y, GLfloat  z, GLfloat  w           )(4f,             x, y, z, w             );
    JOSH3D_SET_UNIFORM(dvec4,              GLdouble x, GLdouble y, GLdouble z, GLdouble w           )(4d,             x, y, z, w             );
    JOSH3D_SET_UNIFORM(ivec4,              GLint    x, GLint    y, GLint    z, GLint    w           )(4i,             x, y, z, w             );
    JOSH3D_SET_UNIFORM(i64vec4_ARB,        GLint64  x, GLint64  y, GLint64  z, GLint64  w           )(4i64ARB,        x, y, z, w             );
    JOSH3D_SET_UNIFORM(uvec4,              GLuint   x, GLuint   y, GLuint   z, GLuint   w           )(4ui,            x, y, z, w             );
    JOSH3D_SET_UNIFORM(u64vec4_ARB,        GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w           )(4ui64ARB,       x, y, z, w             );
    JOSH3D_SET_UNIFORM(floatv,             GLsizei count, const GLfloat*  value                     )(1fv,            count, value           );
    JOSH3D_SET_UNIFORM(doublev,            GLsizei count, const GLdouble* value                     )(1dv,            count, value           );
    JOSH3D_SET_UNIFORM(intv,               GLsizei count, const GLint*    value                     )(1iv,            count, value           );
    JOSH3D_SET_UNIFORM(int64v_ARB,         GLsizei count, const GLint64*  value                     )(1i64vARB,       count, value           );
    JOSH3D_SET_UNIFORM(uintv,              GLsizei count, const GLuint*   value                     )(1uiv,           count, value           );
    JOSH3D_SET_UNIFORM(uint64v_ARB,        GLsizei count, const GLuint64* value                     )(1ui64vARB,      count, value           );
    JOSH3D_SET_UNIFORM(vec2v,              GLsizei count, const GLfloat*  value                     )(2fv,            count, value           );
    JOSH3D_SET_UNIFORM(dvec2v,             GLsizei count, const GLdouble* value                     )(2dv,            count, value           );
    JOSH3D_SET_UNIFORM(ivec2v,             GLsizei count, const GLint*    value                     )(2iv,            count, value           );
    JOSH3D_SET_UNIFORM(i64vec2v_ARB,       GLsizei count, const GLint64*  value                     )(2i64vARB,       count, value           );
    JOSH3D_SET_UNIFORM(uvec2v,             GLsizei count, const GLuint*   value                     )(2uiv,           count, value           );
    JOSH3D_SET_UNIFORM(u64vec2v_ARB,       GLsizei count, const GLuint64* value                     )(2ui64vARB,      count, value           );
    JOSH3D_SET_UNIFORM(vec3v,              GLsizei count, const GLfloat*  value                     )(3fv,            count, value           );
    JOSH3D_SET_UNIFORM(dvec3v,             GLsizei count, const GLdouble* value                     )(3dv,            count, value           );
    JOSH3D_SET_UNIFORM(ivec3v,             GLsizei count, const GLint*    value                     )(3iv,            count, value           );
    JOSH3D_SET_UNIFORM(i64vec3v_ARB,       GLsizei count, const GLint64*  value                     )(3i64vARB,       count, value           );
    JOSH3D_SET_UNIFORM(uvec3v,             GLsizei count, const GLuint*   value                     )(3uiv,           count, value           );
    JOSH3D_SET_UNIFORM(u64vec3v_ARB,       GLsizei count, const GLuint64* value                     )(3ui64vARB,      count, value           );
    JOSH3D_SET_UNIFORM(vec4v,              GLsizei count, const GLfloat*  value                     )(4fv,            count, value           );
    JOSH3D_SET_UNIFORM(dvec4v,             GLsizei count, const GLdouble* value                     )(4dv,            count, value           );
    JOSH3D_SET_UNIFORM(ivec4v,             GLsizei count, const GLint*    value                     )(4iv,            count, value           );
    JOSH3D_SET_UNIFORM(i64vec4v_ARB,       GLsizei count, const GLint64*  value                     )(4i64vARB,       count, value           );
    JOSH3D_SET_UNIFORM(uvec4v,             GLsizei count, const GLuint*   value                     )(4uiv,           count, value           );
    JOSH3D_SET_UNIFORM(u64vec4v_ARB,       GLsizei count, const GLuint64* value                     )(4ui64vARB,      count, value           );
    JOSH3D_SET_UNIFORM(mat2v,              GLsizei count, GLboolean transpose, const GLfloat*  value)(Matrix2fv,      count, transpose, value);
    JOSH3D_SET_UNIFORM(mat2x3v,            GLsizei count, GLboolean transpose, const GLfloat*  value)(Matrix2x3fv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(mat2x4v,            GLsizei count, GLboolean transpose, const GLfloat*  value)(Matrix2x4fv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(dmat2v,             GLsizei count, GLboolean transpose, const GLdouble* value)(Matrix2dv,      count, transpose, value);
    JOSH3D_SET_UNIFORM(dmat2x3v,           GLsizei count, GLboolean transpose, const GLdouble* value)(Matrix2x3dv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(dmat2x4v,           GLsizei count, GLboolean transpose, const GLdouble* value)(Matrix2x4dv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(mat3v,              GLsizei count, GLboolean transpose, const GLfloat*  value)(Matrix3fv,      count, transpose, value);
    JOSH3D_SET_UNIFORM(mat3x2v,            GLsizei count, GLboolean transpose, const GLfloat*  value)(Matrix3x2fv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(mat3x4v,            GLsizei count, GLboolean transpose, const GLfloat*  value)(Matrix3x4fv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(dmat3v,             GLsizei count, GLboolean transpose, const GLdouble* value)(Matrix3dv,      count, transpose, value);
    JOSH3D_SET_UNIFORM(dmat3x2v,           GLsizei count, GLboolean transpose, const GLdouble* value)(Matrix3x2dv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(dmat3x4v,           GLsizei count, GLboolean transpose, const GLdouble* value)(Matrix3x4dv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(mat4v,              GLsizei count, GLboolean transpose, const GLfloat*  value)(Matrix4fv,      count, transpose, value);
    JOSH3D_SET_UNIFORM(mat4x2v,            GLsizei count, GLboolean transpose, const GLfloat*  value)(Matrix4x2fv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(mat4x3v,            GLsizei count, GLboolean transpose, const GLfloat*  value)(Matrix4x3fv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(dmat4v,             GLsizei count, GLboolean transpose, const GLdouble* value)(Matrix4dv,      count, transpose, value);
    JOSH3D_SET_UNIFORM(dmat4x2v,           GLsizei count, GLboolean transpose, const GLdouble* value)(Matrix4x2dv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(dmat4x3v,           GLsizei count, GLboolean transpose, const GLdouble* value)(Matrix4x3dv,    count, transpose, value);
    JOSH3D_SET_UNIFORM(handle_uint64_ARB,  GLuint64 value                                           )(Handleui64ARB,  value                  );
    JOSH3D_SET_UNIFORM(handle_uint64v_ARB, GLsizei count, const GLuint64* values                    )(Handleui64vARB, count, values          );

#undef JOSH3D_SET_UNIFORM
#undef JOSH3D_SET_UNIFORM_RHS

/*
Redundant extensions, most likely no point in using this:

    void set_uniform_1dEXT            (Location location, GLdouble x) const                                                                  { gl::glProgramUniform1dEXT     (self_id(), location, x);              }
    void set_uniform_1fEXT            (Location location, GLfloat v0) const                                                                  { gl::glProgramUniform1fEXT     (self_id(), location, v0);             }
    void set_uniform_1iEXT            (Location location, GLint v0) const                                                                    { gl::glProgramUniform1iEXT     (self_id(), location, v0);             }
    void set_uniform_1i64NV           (Location location, gl::GLint64EXT x) const                                                            { gl::glProgramUniform1i64NV    (self_id(), location, x);              }
    void set_uniform_1ui64NV          (Location location, gl::GLuint64EXT x) const                                                           { gl::glProgramUniform1ui64NV   (self_id(), location, x);              }
    void set_uniform_1uiEXT           (Location location, GLuint v0) const                                                                   { gl::glProgramUniform1uiEXT    (self_id(), location, v0);             }
    void set_uniform_2dEXT            (Location location, GLdouble x, GLdouble y) const                                                      { gl::glProgramUniform2dEXT     (self_id(), location, x, y);           }
    void set_uniform_2fEXT            (Location location, GLfloat v0, GLfloat v1) const                                                      { gl::glProgramUniform2fEXT     (self_id(), location, v0, v1);         }
    void set_uniform_2i64NV           (Location location, gl::GLint64EXT x, gl::GLint64EXT y) const                                          { gl::glProgramUniform2i64NV    (self_id(), location, x, y);           }
    void set_uniform_2iEXT            (Location location, GLint v0, GLint v1) const                                                          { gl::glProgramUniform2iEXT     (self_id(), location, v0, v1);         }
    void set_uniform_2ui64NV          (Location location, gl::GLuint64EXT x, gl::GLuint64EXT y) const                                        { gl::glProgramUniform2ui64NV   (self_id(), location, x, y);           }
    void set_uniform_2uiEXT           (Location location, GLuint v0, GLuint v1) const                                                        { gl::glProgramUniform2uiEXT    (self_id(), location, v0, v1);         }
    void set_uniform_3dEXT            (Location location, GLdouble x, GLdouble y, GLdouble z) const                                          { gl::glProgramUniform3dEXT     (self_id(), location, x, y, z);        }
    void set_uniform_3fEXT            (Location location, GLfloat v0, GLfloat v1, GLfloat v2) const                                          { gl::glProgramUniform3fEXT     (self_id(), location, v0, v1, v2);     }
    void set_uniform_3iEXT            (Location location, GLint v0, GLint v1, GLint v2) const                                                { gl::glProgramUniform3iEXT     (self_id(), location, v0, v1, v2);     }
    void set_uniform_3i64NV           (Location location, gl::GLint64EXT x, gl::GLint64EXT y, gl::GLint64EXT z) const                        { gl::glProgramUniform3i64NV    (self_id(), location, x, y, z);        }
    void set_uniform_3ui64NV          (Location location, gl::GLuint64EXT x, gl::GLuint64EXT y, gl::GLuint64EXT z) const                     { gl::glProgramUniform3ui64NV   (self_id(), location, x, y, z);        }
    void set_uniform_3uiEXT           (Location location, GLuint v0, GLuint v1, GLuint v2) const                                             { gl::glProgramUniform3uiEXT    (self_id(), location, v0, v1, v2);     }
    void set_uniform_4dEXT            (Location location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) const                              { gl::glProgramUniform4dEXT     (self_id(), location, x, y, z, w);     }
    void set_uniform_4fEXT            (Location location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const                              { gl::glProgramUniform4fEXT     (self_id(), location, v0, v1, v2, v3); }
    void set_uniform_4i64NV           (Location location, gl::GLint64EXT x, gl::GLint64EXT y, gl::GLint64EXT z, gl::GLint64EXT w) const      { gl::glProgramUniform4i64NV    (self_id(), location, x, y, z, w);     }
    void set_uniform_4iEXT            (Location location, GLint v0, GLint v1, GLint v2, GLint v3) const                                      { gl::glProgramUniform4iEXT     (self_id(), location, v0, v1, v2, v3); }
    void set_uniform_4ui64NV          (Location location, gl::GLuint64EXT x, gl::GLuint64EXT y, gl::GLuint64EXT z, gl::GLuint64EXT w) const  { gl::glProgramUniform4ui64NV   (self_id(), location, x, y, z, w);     }
    void set_uniform_4uiEXT           (Location location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) const                                  { gl::glProgramUniform4uiEXT    (self_id(), location, v0, v1, v2, v3); }

    void set_uniform_1i64vNV          (Location location, GLsizei count, const gl::GLint64EXT*  value) const                    { gl::glProgramUniform1i64vNV   (self_id(), location, count, value); }
    void set_uniform_1ui64vNV         (Location location, GLsizei count, const gl::GLuint64EXT* value) const                    { gl::glProgramUniform1ui64vNV  (self_id(), location, count, value); }
    void set_uniform_2i64vNV          (Location location, GLsizei count, const gl::GLint64EXT*  value) const                    { gl::glProgramUniform2i64vNV   (self_id(), location, count, value); }
    void set_uniform_2ui64vNV         (Location location, GLsizei count, const gl::GLuint64EXT* value) const                    { gl::glProgramUniform2ui64vNV  (self_id(), location, count, value); }
    void set_uniform_3i64vNV          (Location location, GLsizei count, const gl::GLint64EXT*  value) const                    { gl::glProgramUniform3i64vNV   (self_id(), location, count, value); }
    void set_uniform_3ui64vNV         (Location location, GLsizei count, const gl::GLuint64EXT* value) const                    { gl::glProgramUniform3ui64vNV  (self_id(), location, count, value); }
    void set_uniform_4i64vNV          (Location location, GLsizei count, const gl::GLint64EXT*  value) const                    { gl::glProgramUniform4i64vNV   (self_id(), location, count, value); }
    void set_uniform_4ui64vNV         (Location location, GLsizei count, const gl::GLuint64EXT* value) const                    { gl::glProgramUniform4ui64vNV  (self_id(), location, count, value); }

    void set_uniform_1fvEXT           (Location location, GLsizei count, const GLfloat*         value) const                    { gl::glProgramUniform1fvEXT    (self_id(), location, count, value); }
    void set_uniform_1dvEXT           (Location location, GLsizei count, const GLdouble*        value) const                    { gl::glProgramUniform1dvEXT    (self_id(), location, count, value); }
    void set_uniform_1ivEXT           (Location location, GLsizei count, const GLint*           value) const                    { gl::glProgramUniform1ivEXT    (self_id(), location, count, value); }
    void set_uniform_1uivEXT          (Location location, GLsizei count, const GLuint*          value) const                    { gl::glProgramUniform1uivEXT   (self_id(), location, count, value); }
    void set_uniform_2fvEXT           (Location location, GLsizei count, const GLfloat*         value) const                    { gl::glProgramUniform2fvEXT    (self_id(), location, count, value); }
    void set_uniform_2dvEXT           (Location location, GLsizei count, const GLdouble*        value) const                    { gl::glProgramUniform2dvEXT    (self_id(), location, count, value); }
    void set_uniform_2ivEXT           (Location location, GLsizei count, const GLint*           value) const                    { gl::glProgramUniform2ivEXT    (self_id(), location, count, value); }
    void set_uniform_2uivEXT          (Location location, GLsizei count, const GLuint*          value) const                    { gl::glProgramUniform2uivEXT   (self_id(), location, count, value); }
    void set_uniform_3fvEXT           (Location location, GLsizei count, const GLfloat*         value) const                    { gl::glProgramUniform3fvEXT    (self_id(), location, count, value); }
    void set_uniform_3dvEXT           (Location location, GLsizei count, const GLdouble*        value) const                    { gl::glProgramUniform3dvEXT    (self_id(), location, count, value); }
    void set_uniform_3ivEXT           (Location location, GLsizei count, const GLint*           value) const                    { gl::glProgramUniform3ivEXT    (self_id(), location, count, value); }
    void set_uniform_3uivEXT          (Location location, GLsizei count, const GLuint*          value) const                    { gl::glProgramUniform3uivEXT   (self_id(), location, count, value); }
    void set_uniform_4fvEXT           (Location location, GLsizei count, const GLfloat*         value) const                    { gl::glProgramUniform4fvEXT    (self_id(), location, count, value); }
    void set_uniform_4dvEXT           (Location location, GLsizei count, const GLdouble*        value) const                    { gl::glProgramUniform4dvEXT    (self_id(), location, count, value); }
    void set_uniform_4ivEXT           (Location location, GLsizei count, const GLint*           value) const                    { gl::glProgramUniform4ivEXT    (self_id(), location, count, value); }
    void set_uniform_4uivEXT          (Location location, GLsizei count, const GLuint*          value) const                    { gl::glProgramUniform4uivEXT   (self_id(), location, count, value); }

    void set_uniform_mat2dvEXT        (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const     { gl::glProgramUniformMatrix2dvEXT   (self_id(), location, count, transpose, value); }
    void set_uniform_mat2fvEXT        (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const     { gl::glProgramUniformMatrix2fvEXT   (self_id(), location, count, transpose, value); }
    void set_uniform_mat2x3dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const     { gl::glProgramUniformMatrix2x3dvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat2x3fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const     { gl::glProgramUniformMatrix2x3fvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat2x4dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const     { gl::glProgramUniformMatrix2x4dvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat2x4fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const     { gl::glProgramUniformMatrix2x4fvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat3dvEXT        (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const     { gl::glProgramUniformMatrix3dvEXT   (self_id(), location, count, transpose, value); }
    void set_uniform_mat3fvEXT        (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const     { gl::glProgramUniformMatrix3fvEXT   (self_id(), location, count, transpose, value); }
    void set_uniform_mat3x2dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const     { gl::glProgramUniformMatrix3x2dvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat3x2fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const     { gl::glProgramUniformMatrix3x2fvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat3x4dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const     { gl::glProgramUniformMatrix3x4dvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat3x4fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const     { gl::glProgramUniformMatrix3x4fvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat4dvEXT        (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const     { gl::glProgramUniformMatrix4dvEXT   (self_id(), location, count, transpose, value); }
    void set_uniform_mat4fvEXT        (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const     { gl::glProgramUniformMatrix4fvEXT   (self_id(), location, count, transpose, value); }
    void set_uniform_mat4x2dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const     { gl::glProgramUniformMatrix4x2dvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat4x2fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const     { gl::glProgramUniformMatrix4x2fvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat4x3dvEXT      (Location location, GLsizei count, GLboolean transpose, const GLdouble* value) const     { gl::glProgramUniformMatrix4x3dvEXT (self_id(), location, count, transpose, value); }
    void set_uniform_mat4x3fvEXT      (Location location, GLsizei count, GLboolean transpose, const GLfloat*  value) const     { gl::glProgramUniformMatrix4x3fvEXT (self_id(), location, count, transpose, value); }

    void set_uniform_handleui64NV     (Location location, GLuint64 value) const                                                { gl::glProgramUniformHandleui64NV   (self_id(), location, value); }
    void set_uniform_handleui64vNV    (Location location, GLsizei count, const GLuint64* values) const                         { gl::glProgramUniformHandleui64vNV  (self_id(), location, count, values); }
*/
};


template<typename CRTP>
struct Use
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glUseProgram`.
    [[nodiscard("BindTokens have to be provided to an API call that expects bound state.")]]
    auto use() const
        -> BindToken<Binding::Program>
    {
        return glapi::bind_to_context<Binding::Program>(self_id());
    }
};


template<typename CRTP>
struct Program
    : Uniforms<CRTP>
    , Use<CRTP>
{
    JOSH3D_MIXIN_HEADER

    // Wraps `glAttachShader`.
    template<of_kind<GLKind::Shader> ShaderT>
    void attach_shader(const ShaderT& shader) const requires mt::is_mutable
    {
        gl::glAttachShader(self_id(), decay_to_raw(shader).id());
    }

    // Wraps `glDetachShader`.
    template<of_kind<GLKind::Shader> ShaderT>
    void detach_shader(const ShaderT& shader) const requires mt::is_mutable
    {
        gl::glDetachShader(self_id(), decay_to_raw(shader).id());
    }

    // Wraps `glLinkProgram`.
    void link() const requires mt::is_mutable
    {
        gl::glLinkProgram(self_id());
    }

    // Wraps `glGetProgramiv` with `pname = GL_LINK_STATUS`.
    auto has_linked_successfully() const
        -> bool
    {
        GLboolean is_success;
        gl::glGetProgramiv(self_id(), gl::GL_LINK_STATUS, &is_success);
        return is_success == gl::GL_TRUE;
    }

    // Wraps `glValidateProgram` followed by `glGetProgramiv` with `pname = GL_VALIDATE_STATUS`.
    // Returns `true` if the program is valid according to `glValidateProgram`, `false` otherwise.
    auto validate() const
        -> bool
    {
        gl::glValidateProgram(self_id());
        GLboolean is_valid;
        gl::glGetProgramiv(self_id(), gl::GL_VALIDATE_STATUS, &is_valid);
        return is_valid == gl::GL_TRUE;
    }

    //  Wraps `glGetProgramInfoLog`.
    auto get_info_log() const
        -> std::string
    {
        GLint length_with_null_terminator;
        gl::glGetProgramiv(self_id(), gl::GL_INFO_LOG_LENGTH, &length_with_null_terminator);
        if (length_with_null_terminator <= 0)
            return {};

        std::string log;
        log.resize(size_t(length_with_null_terminator - 1));
        gl::glGetProgramInfoLog(self_id(), length_with_null_terminator, nullptr, log.data());
        return log;
    }

    // Wraps `glGetProgramiv` with `pname = GL_ATTACHED_SHADERS`.
    auto get_num_attached_shaders() const
        -> GLint
    {
        GLint num_shaders;
        gl::glGetProgramiv(self_id(), gl::GL_ATTACHED_SHADERS, &num_shaders);
        return num_shaders;
    }

    // Wraps `glGetProgramiv` with `pname = GL_DELETE_STATUS`.
    auto is_flagged_for_deletion() const
        -> bool
    {
        GLboolean is_flagged;
        gl::glGetProgramiv(self_id(), gl::GL_DELETE_STATUS, &is_flagged);
        return is_flagged == gl::GL_TRUE;
    }
};


} // namespace program_api
} // namespace detail



template<mutability_tag MutT = GLMutable>
class RawProgram
    : public detail::RawGLHandle<>
    , public detail::program_api::Program<RawProgram<MutT>>
{
public:
    static constexpr auto kind_type = GLKind::Program;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawProgram, mutability_traits<RawProgram>, detail::RawGLHandle<>)
};
static_assert(sizeof(RawProgram<GLMutable>) == sizeof(RawProgram<GLConst>));


/*
Default specializations for basic types.
*/
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(bool,      program.set_uniform_int   (loc, v));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(GLint,     program.set_uniform_int   (loc, v));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(GLuint,    program.set_uniform_uint  (loc, v));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(GLfloat,   program.set_uniform_float (loc, v));
JOSH3D_SPECIALIZE_UNIFORM_SET_1ARG(GLdouble,  program.set_uniform_double(loc, v));


} // namespace josh
