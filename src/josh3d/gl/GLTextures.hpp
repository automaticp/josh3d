#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)
#include "GLMutability.hpp"
#include "GLKindHandles.hpp"
#include "GLScalars.hpp"
#include "detail/AndThen.hpp"
#include "detail/AsSelf.hpp"
#include "Size.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include <glbinding/gl/boolean.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/types.h>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <concepts>
#include <type_traits>




namespace josh {


// FIXME: Bound dummies should not be copy-constructable.
// FIXME: Bound dummies should be mutable->const convertable.
template<mutability_tag MutT> class BoundTexture2D;
template<mutability_tag MutT> class BoundTexture2DArray;
template<mutability_tag MutT> class BoundTexture2DMS;
template<mutability_tag MutT> class BoundCubemap;
template<mutability_tag MutT> class BoundCubemapArray;

template<mutability_tag MutT> class RawTexture2D;
template<mutability_tag MutT> class RawTexture2DArray;
template<mutability_tag MutT> class RawTexture2DMS;
template<mutability_tag MutT> class RawCubemap;
template<mutability_tag MutT> class RawCubemapArray;



/*
What follows is truly a product of desparate times.

God help us all.

Hint: scroll to the bottom to the GENERATE_TEXTURE_CLASSES macro
to see how all of this fits together.
*/


/*
Texture specification types, sufficient to create storage for a specific target type.

We use GLenum for type identification in this case,
and some others below.

One alternative is using template-template parameters,
but that turns into a mess rather quickly.
Another is using a custom enum class insead:

enum class GLTexTarget : std::underlying_type_t<GLenum> {
    texture_2d             = to_underlying(gl::GL_TEXTURE_2D),
    texture_2d_array       = to_underlying(gl::GL_TEXTURE_2D_ARRAY),
    texture_2d_multisample = to_underlying(gl::GL_TEXTURE_2D_MULTISAMPLE),
    cubemap                = to_underlying(gl::GL_TEXTURE_CUBE_MAP),
    cubemap_array          = to_underlying(gl::GL_TEXTURE_CUBE_MAP_ARRAY),
};

This would allow us to have custom target types outside OpenGL spec,
but so far it seems unnecessary.
*/


struct TexSpec {
    GLenum internal_format;
    TexSpec(GLenum internal_format)
        : internal_format{ internal_format }
    {}
};

struct TexSpecMS {
    GLenum    internal_format;
    GLsizei   num_samples;
    GLboolean fixed_sample_locations;
    TexSpecMS(GLenum internal_format, GLsizei num_samples, GLboolean fixed_sample_locations)
        : internal_format{ internal_format }, num_samples{ num_samples }
        , fixed_sample_locations{ fixed_sample_locations }
    {}
};


struct TexPackSpec {
    GLenum format, type;
    TexPackSpec(GLenum format, GLenum type)
        : format{ format }, type{ type }
    {}
};




namespace detail {
template<GLenum TargetV> struct GLTexSpecImpl { using type = TexSpec; };
template<> struct GLTexSpecImpl<gl::GL_TEXTURE_2D_MULTISAMPLE> { using type = TexSpecMS; };
template<> struct GLTexSpecImpl<gl::GL_RENDERBUFFER>; // Defined in GLRenderbuffer.

template<GLenum TargetV> struct GLTexSizeImpl;
template<> struct GLTexSizeImpl<gl::GL_TEXTURE_2D>             { using type = Size2I; };
template<> struct GLTexSizeImpl<gl::GL_TEXTURE_2D_ARRAY>       { using type = Size3I; };
template<> struct GLTexSizeImpl<gl::GL_TEXTURE_2D_MULTISAMPLE> { using type = Size2I; };
template<> struct GLTexSizeImpl<gl::GL_TEXTURE_CUBE_MAP>       { using type = Size2I; };
template<> struct GLTexSizeImpl<gl::GL_TEXTURE_CUBE_MAP_ARRAY> { using type = Size3I; };
template<> struct GLTexSizeImpl<gl::GL_RENDERBUFFER>; // Defined in GLRenderbuffer.

} // namespace detail


/*
Texture size type, sufficient to describe dimensions of each target type.
*/
template<GLenum TargetV>
using GLTexSize = detail::GLTexSizeImpl<TargetV>::type;

template<GLenum TargetV>
using GLTexSpec = detail::GLTexSpecImpl<TargetV>::type;







namespace detail {


/*
Implementation trait that enables binding of RawTextureObjects.
*/
template<
    mutability_tag MutT,
    template<typename> typename CRTP,
    template<typename> typename BoundT,
    GLenum TargetV
>
struct BindableTexture {
    BoundT<MutT> bind() const noexcept {
        gl::glBindTexture(TargetV, static_cast<const CRTP<MutT>&>(*this).id());
        return {};
    }

    BoundT<MutT> bind_to_unit(GLenum tex_unit) const noexcept {
        set_active_unit(tex_unit);
        return bind();
    }

    BoundT<MutT> bind_to_unit_index(GLsizei tex_unit_index) const noexcept {
        set_active_unit(gl::GLenum(gl::GL_TEXTURE0 + tex_unit_index));
        return bind();
    }

    static void set_active_unit(GLenum tex_unit) noexcept {
        gl::glActiveTexture(tex_unit);
    }
};


// These exist so that we could 'befriend' the right layer
// that actually constructs the bound handles.
//
// "Friendship is not inherited" - thanks C++.
// Makes it real easy to write implementation traits...
template<mutability_tag MutT> using BindableTexture2D =
    BindableTexture<MutT, RawTexture2D, BoundTexture2D, gl::GL_TEXTURE_2D>;

template<mutability_tag MutT> using BindableTexture2DArray =
    BindableTexture<MutT, RawTexture2DArray, BoundTexture2DArray, gl::GL_TEXTURE_2D_ARRAY>;

template<mutability_tag MutT> using BindableTexture2DMS =
    BindableTexture<MutT, RawTexture2DMS, BoundTexture2DMS, gl::GL_TEXTURE_2D_MULTISAMPLE>;

template<mutability_tag MutT> using BindableCubemap =
    BindableTexture<MutT, RawCubemap, BoundCubemap, gl::GL_TEXTURE_CUBE_MAP>;

template<mutability_tag MutT> using BindableCubemapArray =
    BindableTexture<MutT, RawCubemapArray, BoundCubemapArray, gl::GL_TEXTURE_CUBE_MAP_ARRAY>;




/*
THE texture reflection trait.
*/
template<GLenum TargetV>
struct GLTexInfo {
    static constexpr GLenum target_type = TargetV;
    using size_type = GLTexSize<target_type>;
    using spec_type = GLTexSpec<target_type>;
};








/*
Concepts to verify the properties of RawTextureObjects.
*/


// No value check.
template<typename TextureT>
concept has_tex_target_type = requires {
    { TextureT::target_type } -> same_as_remove_cvref<GLenum>;
};

// Spec can be anything, so we just check existence.
template<typename TextureT>
concept has_tex_spec =
    has_tex_target_type<TextureT> &&
    requires {
        sizeof(GLTexSpec<TextureT::target_type>);
    };

// TexSize is probably SizeN<int>.
template<typename TextureT>
concept has_tex_size =
    has_tex_target_type<TextureT> &&
    requires {
        sizeof(GLTexSize<TextureT::target_type>);
        requires any_of<GLTexSize<TextureT::target_type>, Size2I, Size3I>;
    };


// Tex info has to be of certain format, so we check on TexInfo.
template<typename TextureT>
concept has_tex_info =
    has_tex_target_type<TextureT> &&
    has_tex_spec<TextureT>        &&
    has_tex_size<TextureT>        &&
    requires {
        { GLTexInfo<TextureT::target_type>::target_type } -> same_as_remove_cvref<GLenum>;
        sizeof(GLTexInfo<TextureT::target_type>);
        // Sanity check that the `TextrueT::target_type` is the right value.
        requires (GLTexInfo<TextureT::target_type>::target_type == TextureT::target_type);
        requires std::same_as<typename GLTexInfo<TextureT::target_type>::spec_type, GLTexSpec<TextureT::target_type>>;
        typename GLTexInfo<TextureT::target_type>::size_type;
    };


/*
This concept does not check everything a Texture type should have,
because their interfaces can differ drastically.

It exists to verify accesibility of certain typedefs and
class constants that are primarily useful for reflection in templated code.

Implementation detail for now, but might become useful one day.
*/
template<typename TextureT>
concept gl_texture_object =
    detail::has_tex_spec<TextureT> &&
    detail::has_tex_info<TextureT> &&
    requires(
        TextureT::template object_handle_type_template<GLMutable> mtex,
        TextureT::template object_handle_type_template<GLConst>   ctex)
    {
        // Has TexInfo as a base trait.
        requires std::derived_from<TextureT, detail::GLTexInfo<TextureT::target_type>>;
        // Bindable and unbindable.
        { mtex.bind().unbind() };
        { ctex.bind().unbind() };
        // Bound types have the same TexInfo trait.
        requires std::derived_from<decltype(mtex.bind()), detail::GLTexInfo<TextureT::target_type>>;
        requires std::derived_from<decltype(ctex.bind()), detail::GLTexInfo<TextureT::target_type>>;
        // Bound types are not the same (const/non-const).
        requires !same_as_remove_cvref<decltype(mtex.bind()), decltype(ctex.bind())>;
    };






/*
Next we have all the garbage needed to implement Bound texture dummies.
*/





template<GLenum TargetV>
struct BoundSizeAndSpecGetter {
    GLTexSize<TargetV> get_size(GLint level = 0) {
        using enum GLenum;
        if constexpr (std::same_as<GLTexSize<TargetV>, Size2I>) {
            GLint width, height;
            gl::glGetTexLevelParameteriv(TargetV, level, GL_TEXTURE_WIDTH,  &width);
            gl::glGetTexLevelParameteriv(TargetV, level, GL_TEXTURE_HEIGHT, &height);
            return { width, height };
        } else {
            GLint width, height, depth;
            gl::glGetTexLevelParameteriv(TargetV, level, GL_TEXTURE_WIDTH,  &width);
            gl::glGetTexLevelParameteriv(TargetV, level, GL_TEXTURE_HEIGHT, &height);
            gl::glGetTexLevelParameteriv(TargetV, level, GL_TEXTURE_DEPTH,  &depth);
            return { width, height, depth };
        }
    }

    GLTexSpec<TargetV> get_spec(GLint level = 0) {
        using enum GLenum;
        if constexpr (std::same_as<GLTexSpec<TargetV>, TexSpec>) {
            GLenum internal_format;
            gl::glGetTexLevelParameteriv(
                TargetV, level,
                GL_TEXTURE_INTERNAL_FORMAT, &internal_format
            );
            return { internal_format };
        } else if constexpr (std::same_as<GLTexSpec<TargetV>, TexSpecMS>) {
            GLenum internal_format;
            GLint nsamples;
            GLboolean fixed_sample_locations;
            gl::glGetTexLevelParameteriv(TargetV, level, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);
            gl::glGetTexLevelParameteriv(TargetV, level, GL_TEXTURE_SAMPLES, &nsamples);
            gl::glGetTexLevelParameteriv(TargetV, level, GL_TEXTURE_FIXED_SAMPLE_LOCATIONS, &fixed_sample_locations);
            return { internal_format, nsamples, fixed_sample_locations };
        } else {
            assert(false);
            return { GL_NONE };
        }
    }
};

template<GLenum TargetV>
struct UnbindableTex {
    static void unbind() noexcept { gl::glBindTexture(TargetV, 0); }
};

template<typename CRTP, GLenum TargetV>
struct BoundTexImplConst
    : GLTexInfo<TargetV>
    , BoundSizeAndSpecGetter<TargetV>
    , UnbindableTex<TargetV>
    , AndThen<CRTP>
    , AsSelf<CRTP>
{};

template<typename CRTP, GLenum TargetV>
struct BoundTexImplMutable
    : BoundTexImplConst<CRTP, TargetV>
{
    CRTP& generate_mipmaps() {
        gl::glGenerateMipmap(TargetV);
        return this->as_self();
    }

    CRTP& set_parameter(GLenum param_name, GLint param_value) {
        gl::glTexParameteri(TargetV, param_name, param_value);
        return this->as_self();
    }

    CRTP& set_parameter(GLenum param_name, GLenum param_value) {
        gl::glTexParameteri(TargetV, param_name, param_value);
        return this->as_self();
    }

    CRTP& set_parameter(GLenum param_name, GLfloat param_value) {
        gl::glTexParameterf(TargetV, param_name, param_value);
        return this->as_self();
    }

    CRTP& set_parameter(GLenum param_name, const GLfloat* param_values) {
        gl::glTexParameterfv(TargetV, param_name, param_values);
        return this->as_self();
    }

    CRTP& set_min_mag_filters(GLenum min_filter, GLenum mag_filter)
        requires (!(TargetV == gl::GL_TEXTURE_2D_MULTISAMPLE ||
            TargetV == gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY))
    {
        set_parameter(gl::GL_TEXTURE_MIN_FILTER, min_filter);
        set_parameter(gl::GL_TEXTURE_MAG_FILTER, mag_filter);
        return this->as_self();
    }

    CRTP& set_wrap_st(GLenum wrap_s, GLenum wrap_t)
        requires any_of<typename GLTexInfo<TargetV>::size_type, Size2I, Size3I>
    {
        set_parameter(gl::GL_TEXTURE_WRAP_S, wrap_s);
        set_parameter(gl::GL_TEXTURE_WRAP_T, wrap_t);
        return this->as_self();
    }

    CRTP& set_wrap_str(GLenum wrap_s, GLenum wrap_t, GLenum wrap_r)
        requires std::same_as<typename GLTexInfo<TargetV>::size_type, Size3I>
    {
        set_parameter(gl::GL_TEXTURE_WRAP_S, wrap_s);
        set_parameter(gl::GL_TEXTURE_WRAP_T, wrap_t);
        set_parameter(gl::GL_TEXTURE_WRAP_R, wrap_r);
        return this->as_self();
    }

    CRTP& set_border_color(const GLfloat* colors_array) {
        return set_parameter(gl::GL_TEXTURE_BORDER_COLOR, colors_array);
    }

    CRTP& set_border_color(const glm::vec4& color) {
        return set_border_color(glm::value_ptr(color));
    }

};



/*
Core implementation trait for Bound textures.
*/
template<template<typename> typename BoundTemplateCRTP, mutability_tag MutT>
struct BoundTexImpl;


// GLConst implementations are simple enough.
template<> struct BoundTexImpl<BoundTexture2D, GLConst>      : BoundTexImplConst<BoundTexture2D<GLConst>,      gl::GL_TEXTURE_2D> {};
template<> struct BoundTexImpl<BoundTexture2DArray, GLConst> : BoundTexImplConst<BoundTexture2DArray<GLConst>, gl::GL_TEXTURE_2D_ARRAY> {};
template<> struct BoundTexImpl<BoundTexture2DMS, GLConst>    : BoundTexImplConst<BoundTexture2DMS<GLConst>,    gl::GL_TEXTURE_2D_MULTISAMPLE> {};
template<> struct BoundTexImpl<BoundCubemap, GLConst>        : BoundTexImplConst<BoundCubemap<GLConst>,        gl::GL_TEXTURE_CUBE_MAP> {};
template<> struct BoundTexImpl<BoundCubemapArray, GLConst>   : BoundTexImplConst<BoundCubemapArray<GLConst>,   gl::GL_TEXTURE_CUBE_MAP_ARRAY> {};

// GLMutable need some extra stuff.


inline GLenum best_unpack_format(
    GLenum target, GLenum internal_format) noexcept
{
    GLint format;
    gl::glGetInternalformativ(
        target, internal_format,
        gl::GL_TEXTURE_IMAGE_FORMAT,
        1, &format
    );
    return static_cast<GLenum>(format);
}

inline GLenum best_unpack_type(
    GLenum target, GLenum internal_format) noexcept
{
    GLint type;
    gl::glGetInternalformativ(
        target, internal_format,
        gl::GL_TEXTURE_IMAGE_TYPE,
        1, &type
    );
    return static_cast<GLenum>(type);
}


inline TexPackSpec best_unpack_spec(
    GLenum target, GLenum internal_format) noexcept
{
    return {
        best_unpack_format(target, internal_format),
        best_unpack_type(target, internal_format)
    };
}



inline void tex_image_2d(
    GLenum target, const Size2I& size, const TexSpec& spec,
    const TexPackSpec& pack_spec, const void* data, GLint mipmap_level)
{
    gl::glTexImage2D(
        target, mipmap_level, static_cast<GLint>(spec.internal_format),
        size.width, size.height, 0, pack_spec.format, pack_spec.type, data
    );
}

inline void tex_image_2d_ms(
    GLenum target, const Size2I& size, const TexSpecMS& spec)
{
    gl::glTexImage2DMultisample(
        target, spec.num_samples, spec.internal_format,
        size.width, size.height, spec.fixed_sample_locations
    );
}

inline void tex_image_3d(
    GLenum target, const Size3I& size, const TexSpec& spec,
    const TexPackSpec& pack_spec, const void* data, GLint mipmap_level)
{
    gl::glTexImage3D(
        target, mipmap_level, spec.internal_format,
        size.width, size.height, size.depth, 0, pack_spec.format, pack_spec.type, data
    );
}



template<> struct BoundTexImpl<BoundTexture2D, GLMutable>
    : BoundTexImplMutable<BoundTexture2D<GLMutable>, gl::GL_TEXTURE_2D>
{
    BoundTexture2D<GLMutable>& specify_image(
        const Size2I& size, const TexSpec& spec,
        const TexPackSpec& pack_spec, const void* data,
        GLint mipmap_level = 0)
    {
        tex_image_2d(target_type, size, spec, pack_spec, data, mipmap_level);
        return this->as_self();
    }

    BoundTexture2D<GLMutable>& allocate_image(
        const Size2I& size, const TexSpec& spec,
        GLint mipmap_level = 0)
    {
        return specify_image(
            size, spec,
            // OpenGL specification requires us to provide valid
            // format and type even if no data is uploaded.
            best_unpack_spec(target_type, spec.internal_format), nullptr,
            mipmap_level
        );
    }
};


template<> struct BoundTexImpl<BoundTexture2DArray, GLMutable>
    : BoundTexImplMutable<BoundTexture2DArray<GLMutable>, gl::GL_TEXTURE_2D_ARRAY>
{
    BoundTexture2DArray<GLMutable>& specify_all_images(
        const Size3I& size, const TexSpec& spec,
        const TexPackSpec& pack_spec, const void* data,
        GLint mipmap_level = 0)
    {
        tex_image_3d(target_type, size, spec, pack_spec, data, mipmap_level);
        return this->as_self();
    }

    BoundTexture2DArray<GLMutable>& allocate_all_images(
        const Size3I& size, const TexSpec& spec,
        GLint mipmap_level = 0)
    {
        return specify_all_images(
            size, spec,
            best_unpack_spec(target_type, spec.internal_format), nullptr,
            mipmap_level
        );
    }
};


template<> struct BoundTexImpl<BoundTexture2DMS, GLMutable>
    : BoundTexImplMutable<BoundTexture2DMS<GLMutable>, gl::GL_TEXTURE_2D_MULTISAMPLE>
{
    BoundTexture2DMS<GLMutable>& allocate_image(
        const Size2I& size, const TexSpecMS& spec)
    {
        tex_image_2d_ms(target_type, size, spec);
        return this->as_self();
    }
};


template<> struct BoundTexImpl<BoundCubemap, GLMutable>
    : BoundTexImplMutable<BoundCubemap<GLMutable>, gl::GL_TEXTURE_CUBE_MAP>
{

    BoundCubemap<GLMutable>& specify_face_image(
        GLint face_number,
        const Size2I& size, const TexSpec& spec,
        const TexPackSpec& pack_spec, const void* data,
        GLint mipmap_level = 0)
    {
        GLenum target = gl::GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_number;
        tex_image_2d(target, size, spec, pack_spec, data, mipmap_level);
        return this->as_self();
    }

    BoundCubemap<GLMutable>& specify_all_images(
        const Size2I& size, const TexSpec& spec,
        const TexPackSpec& pack_spec, const void* data,
        GLint mipmap_level = 0)
    {
        for (GLint i{ 0 }; i < 6; ++i) {
            specify_face_image(i, size, spec, pack_spec, data, mipmap_level);
        }
        return this->as_self();
    }

    BoundCubemap<GLMutable>& allocate_all_images(
        const Size2I& size, const TexSpec& spec,
        GLint mipmap_level = 0)
    {
        return specify_all_images(
            size, spec,
            best_unpack_spec(target_type, spec.internal_format), nullptr,
            mipmap_level
        );
    }

};


template<> struct BoundTexImpl<BoundCubemapArray, GLMutable>
    : BoundTexImplMutable<BoundCubemapArray<GLMutable>, gl::GL_TEXTURE_CUBE_MAP_ARRAY>
{
    BoundCubemapArray<GLMutable>& specify_all_images(
        const Size3I& size, const TexSpec& spec,
        const TexPackSpec& pack_spec, const void* data,
        GLint mipmap_level = 0)
    {
        tex_image_3d(
            target_type, { size.width, size.height, 6 * size.depth }, spec,
            pack_spec, data, mipmap_level
        );
        return this->as_self();
    }

    BoundCubemapArray<GLMutable>& allocate_all_images(
        const Size3I& size, const TexSpec& spec,
        GLint mipmap_level = 0)
    {
        return specify_all_images(
            size, spec,
            best_unpack_spec(target_type, spec.internal_format),
            nullptr, mipmap_level
        );
    }

};




} // namespace detail





/*
The magic happens here.
*/
#define JOSH3D_GENERATE_TEXTURE_CLASSES(tex_name, target_enum)                 \
    template<mutability_tag MutT>                                              \
    class Bound##tex_name                                                      \
        : public detail::BoundTexImpl<Bound##tex_name, MutT>                   \
    {                                                                          \
    private:                                                                   \
        friend detail::Bindable##tex_name<MutT>;                               \
        Bound##tex_name() = default;                                           \
    };                                                                         \
                                                                               \
    template<mutability_tag MutT>                                              \
    class Raw##tex_name                                                        \
        : public RawTextureHandle<MutT>                                        \
        , public detail::Bindable##tex_name<MutT>                              \
        , public detail::GLTexInfo<target_enum>                                \
        , public detail::ObjectHandleTypeInfo<Raw##tex_name, MutT>             \
    {                                                                          \
    public:                                                                    \
        JOSH3D_MAGIC_CONSTRUCTORS(Raw##tex_name, MutT, RawTextureHandle<MutT>) \
    };                                                                         \
                                                                               \
static_assert(detail::gl_texture_object<Raw##tex_name<GLConst>>);              \
static_assert(detail::gl_texture_object<Raw##tex_name<GLMutable>>);



JOSH3D_GENERATE_TEXTURE_CLASSES(Texture2D,      gl::GL_TEXTURE_2D)
JOSH3D_GENERATE_TEXTURE_CLASSES(Texture2DArray, gl::GL_TEXTURE_2D_ARRAY)
JOSH3D_GENERATE_TEXTURE_CLASSES(Texture2DMS,    gl::GL_TEXTURE_2D_MULTISAMPLE)
JOSH3D_GENERATE_TEXTURE_CLASSES(Cubemap,        gl::GL_TEXTURE_CUBE_MAP)
JOSH3D_GENERATE_TEXTURE_CLASSES(CubemapArray,   gl::GL_TEXTURE_CUBE_MAP_ARRAY)

#undef JOSH3D_GENERATE_TEXTURE_CLASSES


} // namespace josh
