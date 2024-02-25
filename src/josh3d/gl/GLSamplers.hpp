#pragma once
#include "detail/AndThen.hpp"
#include "GLMutability.hpp" // IWYU pragma: keep (concepts)
#include "GLScalars.hpp"
#include "GLKindHandles.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/types.h>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace josh {


template<mutability_tag MutT> class RawSampler;
template<mutability_tag MutT> class BoundSampler;


template<mutability_tag MutT = GLMutable>
class BoundSampler
    : public detail::AndThen<BoundSampler<MutT>>
{
private:
    GLuint index_;
    friend RawSampler<MutT>;
    BoundSampler(GLuint index) : index_{ index } {}
public:
    static void unbind_at_index(GLuint index) noexcept {
        gl::glBindSampler(index, 0);
    }

    void unbind() const noexcept {
        unbind_at_index(index_);
    }

    GLuint binding_index() const noexcept { return index_; }

};




template<mutability_tag MutT = GLMutable>
class RawSampler
    : public RawSamplerHandle<MutT>
    , public detail::ObjectHandleTypeInfo<RawSampler, MutT>
{
public:
    JOSH3D_MAGIC_CONSTRUCTORS(RawSampler, MutT, RawSamplerHandle<MutT>)


    BoundSampler<MutT> bind_to_unit_index(GLuint unit_index) const {
        gl::glBindSampler(unit_index, this->id());
        return { unit_index };
    }


    void set_parameter(GLenum name, GLint value) const
        requires gl_mutable<MutT>
    {
        gl::glSamplerParameteri(this->id(), name, value);
    }

    void set_parameter(GLenum name, GLenum value) const
        requires gl_mutable<MutT>
    {
        gl::glSamplerParameteri(this->id(), name, value);
    }

    void set_parameter(GLenum name, GLfloat value) const
        requires gl_mutable<MutT>
    {
        gl::glSamplerParameterf(this->id(), name, value);
    }

    void set_parameter(GLenum name, const GLfloat* value) const
        requires gl_mutable<MutT>
    {
        gl::glSamplerParameterfv(this->id(), name, value);
    }


    void set_min_mag_filters(GLenum min_filter, GLenum mag_filter) const {
        set_parameter(gl::GL_TEXTURE_MIN_FILTER, min_filter);
        set_parameter(gl::GL_TEXTURE_MAG_FILTER, mag_filter);
    }


    void set_min_max_lod(GLfloat min_lod, GLfloat max_lod) const {
        set_parameter(gl::GL_TEXTURE_MIN_LOD, min_lod);
        set_parameter(gl::GL_TEXTURE_MAX_LOD, max_lod);
    }


    void set_wrap_st(GLenum wrap_s, GLenum wrap_t) const {
        set_parameter(gl::GL_TEXTURE_WRAP_S, wrap_s);
        set_parameter(gl::GL_TEXTURE_WRAP_T, wrap_t);
    }

    void set_wrap_st(GLenum wrap_st) const {
        set_wrap_st(wrap_st, wrap_st);
    }

    void set_wrap_str(GLenum wrap_s, GLenum wrap_t, GLenum wrap_r) const {
        set_parameter(gl::GL_TEXTURE_WRAP_S, wrap_s);
        set_parameter(gl::GL_TEXTURE_WRAP_T, wrap_t);
        set_parameter(gl::GL_TEXTURE_WRAP_R, wrap_r);
    }

    void set_wrap_str(GLenum wrap_str) const {
        set_wrap_str(wrap_str, wrap_str, wrap_str);
    }

    void set_border_color(const GLfloat* colors_array) const {
        set_parameter(gl::GL_TEXTURE_BORDER_COLOR, colors_array);
    }

    void set_border_color(const glm::vec4& color) const {
        set_border_color(glm::value_ptr(color));
    }


    void set_compare_mode(GLenum mode) const {
        set_parameter(gl::GL_TEXTURE_COMPARE_MODE, mode);
    }

    void set_compare_mode_none() const {
        set_compare_mode(gl::GL_NONE);
    }

    void set_compare_mode_reference() const {
        set_compare_mode(gl::GL_COMPARE_REF_TO_TEXTURE);
    }

    void set_compare_func(GLenum func) const {
        set_parameter(gl::GL_TEXTURE_COMPARE_FUNC, func);
    }

};




} // namespace josh
