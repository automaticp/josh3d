#pragma once
#include "GLAPI.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/MixinHeaderMacro.hpp"
#include "detail/RawGLHandle.hpp"


namespace josh {
namespace detail {
namespace sampler_api {


template<typename CRTP>
struct Bind
{
    JOSH3D_MIXIN_HEADER

    [[nodiscard("Discarding bound state is error-prone. Consider using BindGuard to automate unbinding.")]]
    auto bind_to_texture_unit(GLuint unit_index) const -> BindToken<BindingI::Sampler>
    {
        return glapi::bind_to_context<BindingI::Sampler>(unit_index, self_id());
    }
};


template<typename CRTP>
struct Parameters
{
    JOSH3D_MIXIN_HEADER

    // Compare Func.

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_COMPARE_FUNC`.
    void set_compare_func(CompareOp compare_func) const requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_COMPARE_FUNC, enum_cast<GLenum>(compare_func));
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_COMPARE_FUNC`.
    auto get_compare_func() const -> CompareOp
    {
        GLenum op;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_COMPARE_FUNC, &op);
        return enum_cast<CompareOp>(op);
    }


    // Compare Mode.

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_COMPARE_MODE`.
    // Passes `GL_COMPARE_REF_TO_TEXTURE` if `enable_compare_mode` is `true`, `GL_NONE` otherwise.
    void set_compare_ref_depth_to_texture(bool enable_compare_mode) const requires mt::is_mutable
    {
        gl::glSamplerParameteri(
            self_id(), gl::GL_TEXTURE_COMPARE_MODE,
            enable_compare_mode ? gl::GL_COMPARE_REF_TO_TEXTURE : gl::GL_NONE
        );
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_COMPARE_MODE`.
    // Returns `true` if the result is `GL_COMPARE_REF_TO_TEXTURE`, `false` otherwise.
    auto get_compare_ref_depth_to_texture() const -> bool
    {
        GLenum mode;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_COMPARE_MODE, mode);
        return mode == gl::GL_COMPARE_REF_TO_TEXTURE;
    }


    // LOD Bias.

    // Wraps `glSamplerParameterf` with `pname = GL_TEXTURE_LOD_BIAS`.
    void set_lod_bias(GLfloat bias) const requires mt::is_mutable
    {
        gl::glSamplerParameterf(self_id(), gl::GL_TEXTURE_LOD_BIAS, bias);
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_LOD_BIAS`.
    auto get_lod_bias() const -> GLfloat
    {
        GLfloat bias;
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_LOD_BIAS, &bias);
        return bias;
    }


    // Min/Max LOD.

    // Wraps `glSamplerParameterf` with `pname = GL_TEXTURE_MIN_LOD`.
    void set_min_lod(GLfloat min_lod) const requires mt::is_mutable
    {
        gl::glSamplerParameterf(self_id(), gl::GL_TEXTURE_MIN_LOD, min_lod);
    }

    // Wraps `glSamplerParameterf` with `pname = GL_TEXTURE_MAX_LOD`.
    void set_max_lod(GLfloat max_lod) const requires mt::is_mutable
    {
        gl::glSamplerParameterf(self_id(), gl::GL_TEXTURE_MAX_LOD, max_lod);
    }

    // Wraps `glSamplerParameterf` with `pname = GL_TEXTURE_[MIN|MAX]_LOD`.
    void set_min_max_lod(GLfloat min_lod, GLfloat max_lod) const requires mt::is_mutable
    {
        set_min_lod(min_lod);
        set_max_lod(max_lod);
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_MIN_LOD`.
    auto get_min_lod() const -> GLfloat
    {
        GLfloat min_lod;
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_MIN_LOD, &min_lod);
        return min_lod;
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_MAX_LOD`.
    auto get_max_lod() const -> GLfloat
    {
        GLfloat max_lod;
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_MAX_LOD, &max_lod);
        return max_lod;
    }


    // Max Anisotropy.

    // Wraps `glSamplerParameterf` with `pname = GL_TEXTURE_MAX_ANISOTROPY`.
    void set_max_anisotropy(GLfloat max_anisotropy) const requires mt::is_mutable
    {
        gl::glSamplerParameterf(self_id(), gl::GL_TEXTURE_MAX_ANISOTROPY, max_anisotropy);
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_MAX_ANISOTROPY`.
    auto get_max_anisotropy() const -> GLfloat
    {
        GLfloat max_anisotropy;
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);
        return max_anisotropy;
    }


    // Min/Mag Filters.

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_MIN_FILTER`.
    void set_min_filter(MinFilter min_filter) const requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_MIN_FILTER, enum_cast<GLenum>(min_filter));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_MIN_FILTER`.
    void set_min_filter(MinFilterNoLOD min_filter) const requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_MIN_FILTER, enum_cast<GLenum>(min_filter));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_MAG_FILTER`.
    void set_mag_filter(MagFilter mag_filter) const requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_MAG_FILTER, enum_cast<GLenum>(mag_filter));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_[MIN|MAG]_FILTER`.
    void set_min_mag_filters(MinFilter min_filter, MagFilter mag_filter) const requires mt::is_mutable
    {
        set_min_filter(min_filter);
        set_mag_filter(mag_filter);
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_[MIN|MAG]_FILTER`.
    void set_min_mag_filters(MinFilterNoLOD min_filter, MagFilter mag_filter) const requires mt::is_mutable
    {
        set_min_filter(min_filter);
        set_mag_filter(mag_filter);
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_MIN_FILTER`.
    auto get_min_filter() const -> MinFilter
    {
        GLenum result;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_MIN_FILTER, &result);
        return enum_cast<MinFilter>(result);
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_MAG_FILTER`.
    auto get_mag_filter() const -> MagFilter
    {
        GLenum result;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_MAG_FILTER, &result);
        return enum_cast<MagFilter>(result);
    }


    // Border Color.

    // Wraps `glSamplerParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_unorm(const RGBAUNorm& rgba) const requires mt::is_mutable
    {
        gl::glSamplerParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, &rgba.r);
    }

    // Wraps `glSamplerParameteriv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_snorm(const RGBASNorm& rgba) const requires mt::is_mutable
    {
        gl::glSamplerParameteriv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, &rgba.r);
    }

    // Wraps `glSamplerParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_float(const RGBAF& rgba) const requires mt::is_mutable
    {
        gl::glSamplerParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, &rgba.r);
    }

    // Wraps `glSamplerParameterIiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_int(const RGBAI& rgba) const requires mt::is_mutable
    {
        gl::glSamplerParameterIiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, &rgba.r);
    }

    // Wraps `glSamplerParameterIuiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    void set_border_color_uint(const RGBAUI& rgba) const requires mt::is_mutable
    {
        gl::glSamplerParameterIuiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, &rgba.r);
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_border_color_unorm() const -> RGBAUNorm
    {
        GLfloat rgba[4];
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_border_color_snorm() const -> RGBASNorm
    {
        GLint rgba[4];
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetSamplerParameterfv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_border_color_float() const -> RGBAF
    {
        GLfloat rgba[4];
        gl::glGetSamplerParameterfv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetSamplerParameterIiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_border_color_int() const -> RGBAI
    {
        GLint rgba[4];
        gl::glGetSamplerParameterIiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }

    // Wraps `glGetSamplerParameterIuiv` with `pname = GL_TEXTURE_BORDER_COLOR`.
    auto get_border_color_uint() const -> RGBAUI
    {
        GLuint rgba[4];
        gl::glGetSamplerParameterIuiv(self_id(), gl::GL_TEXTURE_BORDER_COLOR, rgba);
        return { rgba[0], rgba[1], rgba[2], rgba[3] };
    }


    // Wrap.

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_WRAP_S`.
    void set_wrap_s(Wrap wrap_s) const requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_WRAP_S, enum_cast<GLenum>(wrap_s));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_WRAP_T`.
    void set_wrap_t(Wrap wrap_t) const requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_WRAP_T, enum_cast<GLenum>(wrap_t));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_WRAP_R`.
    void set_wrap_r(Wrap wrap_r) const requires mt::is_mutable
    {
        gl::glSamplerParameteri(self_id(), gl::GL_TEXTURE_WRAP_R, enum_cast<GLenum>(wrap_r));
    }

    // Wraps `glSamplerParameteri` with `pname = GL_TEXTURE_WRAP_[S|T|R]`.
    void set_wrap_all(Wrap wrap_str) const requires mt::is_mutable
    {
        set_wrap_s(wrap_str);
        set_wrap_t(wrap_str);
        set_wrap_r(wrap_str);
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_WRAP_S`.
    auto get_wrap_s() const -> Wrap
    {
        GLenum wrap;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_WRAP_S, &wrap);
        return enum_cast<Wrap>(wrap);
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_WRAP_T`.
    auto get_wrap_t() const -> Wrap
    {
        GLenum wrap;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_WRAP_T, &wrap);
        return enum_cast<Wrap>(wrap);
    }

    // Wraps `glGetSamplerParameteriv` with `pname = GL_TEXTURE_WRAP_R`.
    auto get_wrap_r() const -> Wrap
    {
        GLenum wrap;
        gl::glGetSamplerParameteriv(self_id(), gl::GL_TEXTURE_WRAP_R, &wrap);
        return enum_cast<Wrap>(wrap);
    }
};


template<typename CRTP>
struct Sampler
    : Bind<CRTP>
    , Parameters<CRTP>
{};


} // namespace sampler_api
} // namspace detail


template<mutability_tag MutT = GLMutable>
class RawSampler
    : public detail::RawGLHandle<>
    , public detail::sampler_api::Sampler<RawSampler<MutT>>
{
public:
    static constexpr auto kind_type = GLKind::Sampler;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawSampler, mutability_traits<RawSampler>, detail::RawGLHandle<>)
};




} // namespace josh
