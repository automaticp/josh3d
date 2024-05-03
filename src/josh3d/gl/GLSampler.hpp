#pragma once
#include "GLAPI.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"




namespace josh {



namespace detail {


template<typename CRTP>
struct SamplerDSAInterface_Bind {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); } \
    using mt = mutability_traits<CRTP>;
public:

    [[nodiscard("Discarding bound state is error-prone. Consider using BindGuard to automate unbinding.")]]
    auto bind_to_texture_unit(GLuint unit_index) const noexcept
        -> BindToken<BindingIndexed::Sampler>
    {
        gl::glBindSampler(unit_index, self_id());
        return { self_id(), unit_index };
    }

};






template<typename CRTP>
struct SamplerDSAInterface_Parameters {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); } \
    using mt = mutability_traits<CRTP>;
public:



    // Compare Func.

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_COMPARE_FUNC`.
    void set_compare_func(CompareOp compare_func) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_COMPARE_FUNC, enum_cast<GLenum>(compare_func));
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_COMPARE_FUNC`.
    auto get_compare_func() const noexcept
        -> CompareOp
    {
        GLenum op;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_COMPARE_FUNC, &op);
        return enum_cast<CompareOp>(op);
    }







    // Compare Mode.

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_COMPARE_MODE`.
    // Passes `GL_COMPARE_REF_TO_TEXTURE` if `enable_compare_mode` is `true`, `GL_NONE` otherwise.
    void set_compare_ref_depth_to_texture(bool enable_compare_mode) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameteri(
            self_id(), gl::GL_TEXTURE_COMPARE_MODE,
            enable_compare_mode ? gl::GL_COMPARE_REF_TO_TEXTURE : gl::GL_NONE
        );
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_COMPARE_MODE`.
    // Returns `true` if the result is `GL_COMPARE_REF_TO_TEXTURE`, `false` otherwise.
    bool get_compare_ref_depth_to_texture() const noexcept {
        GLenum mode;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_COMPARE_MODE, mode);
        return mode == gl::GL_COMPARE_REF_TO_TEXTURE;
    }









    // LOD Bias.

    // Wraps `glSamplerParameterf` with `pname = GL_TEXTURE_LOD_BIAS`.
    void set_lod_bias(GLfloat bias) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameterf(self_id(), gl::GL_TEXTURE_LOD_BIAS, bias);
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_LOD_BIAS`.
    auto get_lod_bias() const noexcept
        -> GLfloat
    {
        GLfloat bias;
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_LOD_BIAS, &bias);
        return bias;
    }









    // Min/Max LOD.

    // Wraps `glSamplerParameterf` with `pname = GL_TEXTURE_MIN_LOD`.
    void set_min_lod(GLfloat min_lod) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameterf(self_id(), gl::GL_TEXTURE_MIN_LOD, min_lod);
    }

    // Wraps `glSamplerParameterf` with `pname = GL_TEXTURE_MAX_LOD`.
    void set_max_lod(GLfloat max_lod) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameterf(self_id(), gl::GL_TEXTURE_MAX_LOD, max_lod);
    }

    // Wraps `glSamplerParameterf` with `pname = GL_TEXTURE_[MIN|MAX]_LOD`.
    void set_min_max_lod(GLfloat min_lod, GLfloat max_lod) const noexcept
        requires mt::is_mutable
    {
        set_min_lod(min_lod);
        set_max_lod(max_lod);
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_MIN_LOD`.
    auto get_min_lod() const noexcept
        -> GLfloat
    {
        GLfloat min_lod;
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_MIN_LOD, &min_lod);
        return min_lod;
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_MAX_LOD`.
    auto get_max_lod() const noexcept
        -> GLfloat
    {
        GLfloat max_lod;
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_MAX_LOD, &max_lod);
        return max_lod;
    }









    // Max Anisotropy.

    // Wraps `glSamplerParameterf` with `pname = GL_TEXTURE_MAX_ANISOTROPY`.
    void set_max_anisotropy(GLfloat max_anisotropy) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameterf(self_id(), gl::GL_TEXTURE_MAX_ANISOTROPY, max_anisotropy);
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_MAX_ANISOTROPY`.
    auto get_max_anisotropy() const noexcept
        -> GLfloat
    {
        GLfloat max_anisotropy;
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);
        return max_anisotropy;
    }







    // Min/Mag Filters.

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_MIN_FILTER`.
    void set_min_filter(MinFilter min_filter) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_MIN_FILTER, enum_cast<GLenum>(min_filter));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_MIN_FILTER`.
    void set_min_filter(MinFilterNoLOD min_filter) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_MIN_FILTER, enum_cast<GLenum>(min_filter));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_MAG_FILTER`.
    void set_mag_filter(MagFilter mag_filter) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_MAG_FILTER, enum_cast<GLenum>(mag_filter));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_[MIN|MAG]_FILTER`.
    void set_min_mag_filters(MinFilter min_filter, MagFilter mag_filter) const noexcept
        requires mt::is_mutable
    {
        set_min_filter(min_filter);
        set_mag_filter(mag_filter);
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_[MIN|MAG]_FILTER`.
    void set_min_mag_filters(MinFilterNoLOD min_filter, MagFilter mag_filter) const noexcept
        requires mt::is_mutable
    {
        set_min_filter(min_filter);
        set_mag_filter(mag_filter);
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_MIN_FILTER`.
    auto get_min_filter() const noexcept
        -> MinFilter
    {
        GLenum result;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_MIN_FILTER, &result);
        return enum_cast<MinFilter>(result);
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_MAG_FILTER`.
    auto get_mag_filter() const noexcept
        -> MagFilter
    {
        GLenum result;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_MAG_FILTER, &result);
        return enum_cast<MagFilter>(result);
    }










    // Wraps `glSamplerParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_unorm(GLfloat r, GLfloat g, GLfloat b, GLfloat a) const noexcept
        requires mt::is_mutable
    {
        GLfloat rgbaf[4]{ r, g, b, a };
        gl::glSamplerParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgbaf);
    }

    // Wraps `glSamplerParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_unorm(RGBAUNorm rgba) const noexcept
        requires mt::is_mutable
    {
        set_border_color_unorm(rgba.r, rgba.g, rgba.b, rgba.a);
    }

    // Wraps `glSamplerParameteriv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_snorm(GLint r, GLint g, GLint b, GLint a) const noexcept
        requires mt::is_mutable
    {
        GLint rgba_snorm[4]{ r, g, b, a };
        gl::glSamplerParameteriv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba_snorm);
    }

    // Wraps `glSamplerParameteriv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_snorm(RGBASNorm rgba) const noexcept
        requires mt::is_mutable
    {
        set_border_color_snorm(rgba.r, rgba.g, rgba.b, rgba.a);
    }

    // Wraps `glSamplerParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_float(GLfloat r, GLfloat g, GLfloat b, GLfloat a) const noexcept
        requires mt::is_mutable
    {
        GLfloat rgbaf[4]{ r, g, b, a };
        gl::glSamplerParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgbaf);
    }

    // Wraps `glSamplerParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_float(RGBAF rgba) const noexcept
        requires mt::is_mutable
    {
        set_border_color_float(rgba.r, rgba.g, rgba.b, rgba.a);
    }


    // Wraps `glSamplerParameterIiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_integer(GLint r, GLint g, GLint b, GLint a) const noexcept
        requires mt::is_mutable
    {
        GLint rgbai[4]{ r, g, b, a };
        gl::glSamplerParameterIiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgbai);
    }

    // Wraps `glSamplerParameterIiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_integer(RGBAI rgba) const noexcept
        requires mt::is_mutable
    {
        set_border_color_integer(rgba.r, rgba.g, rgba.b, rgba.a);
    }

    // Wraps `glSamplerParameterIuiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_unsigned_integer(GLuint r, GLuint g, GLuint b, GLuint a) const noexcept
        requires mt::is_mutable
    {
        GLuint rgbaui[4]{ r, g, b, a };
        gl::glSamplerParameterIuiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgbaui);
    }

    // Wraps `glSamplerParameterIuiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_unsigned_integer(RGBAUI rgba) const noexcept
        requires mt::is_mutable
    {
        set_border_color_unsigned_integer(rgba.r, rgba.g, rgba.b, rgba.a);
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_border_color_unorm() const noexcept
        -> RGBAUNorm
    {
        GLfloat rgba[4];
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_border_color_snorm() const noexcept
        -> RGBASNorm
    {
        GLint rgba[4];
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_border_color_float() const noexcept
        -> RGBAF
    {
        GLfloat rgba[4];
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetSamplerParameterIiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_border_color_integer() const noexcept
        -> RGBAI
    {
        GLint rgba[4];
        gl::glGetSamplerParameterIiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetSamplerParameterIuiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_border_color_unsigned_integer() const noexcept
        -> RGBAUI
    {
        GLuint rgba[4];
        gl::glGetSamplerParameterIuiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }







    // Wrap.

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_WRAP_S`.
    void set_wrap_s(Wrap wrap_s) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_WRAP_S, enum_cast<GLenum>(wrap_s));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_WRAP_T`.
    void set_wrap_t(Wrap wrap_t) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_WRAP_T, enum_cast<GLenum>(wrap_t));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_WRAP_R`.
    void set_wrap_r(Wrap wrap_r) const noexcept
        requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_WRAP_R, enum_cast<GLenum>(wrap_r));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_WRAP_[S|T|R]`.
    void set_wrap_all(Wrap wrap_str) const noexcept
        requires mt::is_mutable
    {
        set_wrap_s(wrap_str);
        set_wrap_t(wrap_str);
        set_wrap_r(wrap_str);
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_WRAP_S`.
    Wrap get_wrap_s() const noexcept {
        GLenum wrap;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_WRAP_S, &wrap);
        return enum_cast<Wrap>(wrap);
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_WRAP_T`.
    Wrap get_wrap_t() const noexcept {
        GLenum wrap;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_WRAP_T, &wrap);
        return enum_cast<Wrap>(wrap);
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_WRAP_R`.
    Wrap get_wrap_r() const noexcept {
        GLenum wrap;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_WRAP_R, &wrap);
        return enum_cast<Wrap>(wrap);
    }

};



template<typename CRTP>
struct SamplerDSAInterface
    : SamplerDSAInterface_Bind<CRTP>
    , SamplerDSAInterface_Parameters<CRTP>
{};




} // namspace detail










template<mutability_tag MutT = GLMutable>
class RawSampler
    : public detail::RawGLHandle<MutT>
    , public detail::SamplerDSAInterface<RawSampler<MutT>>
{
public:
    static constexpr GLKind kind_type = GLKind::Sampler;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawSampler, mutability_traits<RawSampler>, detail::RawGLHandle<MutT>)
};




} // namespace josh
