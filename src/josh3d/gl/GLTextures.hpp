#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)
#include "GLMutability.hpp"
#include "RawGLHandles.hpp"
#include "GLScalars.hpp"
#include "AndThen.hpp"
#include "AsSelf.hpp"
#include "Size.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/types.h>
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
What follows is truly a product of insanity.

God help us all.

Hint: scroll to the bottom to the GENERATE_TEXTURE_CLASSES macro
to see how all of this fits together.
*/




/*
On the topic of what would make sense to actually consider to be a const operation:

1. Modification of a property of an OpenGL object specifically: writing to/resizing buffers,
changing draw hints, parameters, etc. - is a non-const operation.

2. Operation that modifies an OpenGL context but not the properties of objects: binding,
changing active units, buffer bindnigs, etc. - _can_ be considered a const operation.

3. Read operation on an object: getting properties, validation, etc - is a const operation.

The point 2 is probably the most important one, as without it, you can't really do anything
useful and still preserve some sense of const-correctness. If I can't even bind a texture
for sampling (reading) when it's const, then that const handle is useless for me.

Yes, you can still abuse this and find ways to write to the texture even when binding
a const handle, but, like, don't, ok? You can just write raw OpenGL if you want it that much.

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
template<GLenum TargetV>
struct GLTexSpec;


template<> struct GLTexSpec<gl::GL_TEXTURE_2D> {
    GLenum internal_format, format, type;
    GLTexSpec(GLenum internal_format, GLenum format, GLenum type)
        : internal_format{ internal_format }, format{ format }, type{ type }
    {}
};


template<> struct GLTexSpec<gl::GL_TEXTURE_2D_ARRAY> {
    GLenum internal_format, format, type;
    GLTexSpec(GLenum internal_format, GLenum format, GLenum type)
        : internal_format{ internal_format }, format{ format }, type{ type }
    {}
};


template<> struct GLTexSpec<gl::GL_TEXTURE_2D_MULTISAMPLE> {
    GLenum    internal_format;
    GLsizei   nsamples;
    GLboolean fixed_sample_locations;
    GLTexSpec(GLenum internal_format, GLsizei nsamples, GLboolean fixed_sample_locations)
        : internal_format{ internal_format }, nsamples{ nsamples }
        , fixed_sample_locations{ fixed_sample_locations }
    {}
};


template<> struct GLTexSpec<gl::GL_TEXTURE_CUBE_MAP> {
    GLenum internal_format, format, type;
    GLTexSpec(GLenum internal_format, GLenum format, GLenum type)
        : internal_format{ internal_format }, format{ format }, type{ type }
    {}
};


template<> struct GLTexSpec<gl::GL_TEXTURE_CUBE_MAP_ARRAY> {
    GLenum internal_format, format, type;
    GLTexSpec(GLenum internal_format, GLenum format, GLenum type)
        : internal_format{ internal_format }, format{ format }, type{ type }
    {}
};



namespace detail {
template<GLenum TargetV> struct GLTexSizeImpl;
template<> struct GLTexSizeImpl<gl::GL_TEXTURE_2D>             { using type = Size2I; };
template<> struct GLTexSizeImpl<gl::GL_TEXTURE_2D_ARRAY>       { using type = Size3I; };
template<> struct GLTexSizeImpl<gl::GL_TEXTURE_2D_MULTISAMPLE> { using type = Size2I; };
template<> struct GLTexSizeImpl<gl::GL_TEXTURE_CUBE_MAP>       { using type = Size2I; };
template<> struct GLTexSizeImpl<gl::GL_TEXTURE_CUBE_MAP_ARRAY> { using type = Size3I; };
} // namespace detail


/*
Texture size type, sufficient to describe dimensions of each target type.
*/
template<GLenum TargetV>
using GLTexSize = detail::GLTexSizeImpl<TargetV>::type;









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
        TextureT::object_handle_mutable_type mtex,
        TextureT::object_handle_const_type   ctex)
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
struct UnbindableTex {
    static void unbind() noexcept { gl::glBindTexture(TargetV, 0); }
};

template<typename CRTP, GLenum TargetV>
struct BoundTexImplCommon
    : GLTexInfo<TargetV>
    , UnbindableTex<TargetV>
    , AndThen<CRTP>
    , AsSelf<CRTP>
{};

template<typename CRTP, GLenum TargetV>
struct BoundTexImplMutable
    : BoundTexImplCommon<CRTP, TargetV>
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
};



/*
Core implementation trait for Bound textures.
*/
template<template<typename> typename BoundTemplateCRTP, mutability_tag MutT>
struct BoundTexImpl;


// GLConst implementations are simple enough.
template<> struct BoundTexImpl<BoundTexture2D, GLConst>      : BoundTexImplCommon<BoundTexture2D<GLConst>,      gl::GL_TEXTURE_2D> {};
template<> struct BoundTexImpl<BoundTexture2DArray, GLConst> : BoundTexImplCommon<BoundTexture2DArray<GLConst>, gl::GL_TEXTURE_2D_ARRAY> {};
template<> struct BoundTexImpl<BoundTexture2DMS, GLConst>    : BoundTexImplCommon<BoundTexture2DMS<GLConst>,    gl::GL_TEXTURE_2D_MULTISAMPLE> {};
template<> struct BoundTexImpl<BoundCubemap, GLConst>        : BoundTexImplCommon<BoundCubemap<GLConst>,        gl::GL_TEXTURE_CUBE_MAP> {};
template<> struct BoundTexImpl<BoundCubemapArray, GLConst>   : BoundTexImplCommon<BoundCubemapArray<GLConst>,   gl::GL_TEXTURE_CUBE_MAP_ARRAY> {};

// GLMutable need some extra stuff.
template<> struct BoundTexImpl<BoundTexture2D, GLMutable>
    : BoundTexImplMutable<BoundTexture2D<GLMutable>, gl::GL_TEXTURE_2D>
{
    BoundTexture2D<GLMutable>& specify_image(
        const size_type& size, const spec_type& spec,
        const void* data, GLint mipmap_level = 0)
    {
        gl::glTexImage2D(
            target_type, mipmap_level, static_cast<GLint>(spec.internal_format),
            size.width, size.height, 0, spec.format, spec.type, data
        );
        return this->as_self();
    }
};


template<> struct BoundTexImpl<BoundTexture2DArray, GLMutable>
    : BoundTexImplMutable<BoundTexture2DArray<GLMutable>, gl::GL_TEXTURE_2D_ARRAY>
{
    BoundTexture2DArray<GLMutable>& specify_all_images(
        const size_type& size, const spec_type& spec,
        const void* data, GLint mipmap_level = 0)
    {
        gl::glTexImage3D(
            target_type, mipmap_level, spec.internal_format,
            size.width, size.height, size.depth, 0, spec.format, spec.type, data
        );
        return this->as_self();
    }
};


template<> struct BoundTexImpl<BoundTexture2DMS, GLMutable>
    : BoundTexImplMutable<BoundTexture2DMS<GLMutable>, gl::GL_TEXTURE_2D_MULTISAMPLE>
{
    BoundTexture2DMS<GLMutable>& specify_image(
        const size_type& size, const spec_type& spec)
    {
        gl::glTexImage2DMultisample(
            target_type, spec.nsamples, spec.internal_format,
            size.width, size.height, spec.fixed_sample_locations
        );
        return this->as_self();
    }
};


template<> struct BoundTexImpl<BoundCubemap, GLMutable>
    : BoundTexImplMutable<BoundCubemap<GLMutable>, gl::GL_TEXTURE_CUBE_MAP>
{
    BoundCubemap<GLMutable>& specify_image(
        GLenum target,
        const size_type& size, const spec_type& spec,
        const void* data, GLint mipmap_level = 0)
    {
        gl::glTexImage2D(
            target, mipmap_level, spec.internal_format,
            size.width, size.height, 0, spec.format, spec.type, data
        );
        return this->as_self();
    }

    BoundCubemap<GLMutable>& specify_all_images(
        const size_type& size, const spec_type& spec,
        const void* data, GLint mipmap_level = 0)
    {
        for (size_t i{ 0 }; i < 6; ++i) {
            specify_image(
                gl::GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                size, spec, data, mipmap_level
            );
        }
        return this->as_self();
    }
};


template<> struct BoundTexImpl<BoundCubemapArray, GLMutable>
    : BoundTexImplMutable<BoundCubemapArray<GLMutable>, gl::GL_TEXTURE_CUBE_MAP_ARRAY>
{
    BoundCubemapArray<GLMutable>& specify_all_images(
        const size_type& size, const spec_type& spec,
        const void* data, GLint mipmap_level = 0)
    {
        gl::glTexImage3D(
            target_type, mipmap_level, spec.internal_format,
            size.width, size.height, 6 * size.depth, 0, spec.format, spec.type, data
        );
        return this->as_self();
    }
};




} // namespace detail





/*
The magic happens here.
*/
#define GENERATE_TEXTURE_CLASSES(tex_name, target_enum)            \
    template<mutability_tag MutT>                                  \
    class Bound##tex_name                                          \
        : public detail::BoundTexImpl<Bound##tex_name, MutT>       \
    {                                                              \
    private:                                                       \
        friend detail::Bindable##tex_name<MutT>;                   \
        Bound##tex_name() = default;                               \
    };                                                             \
                                                                   \
    template<mutability_tag MutT>                                  \
    class Raw##tex_name                                            \
        : public RawTextureHandle<MutT>                            \
        , public detail::Bindable##tex_name<MutT>                  \
        , public detail::GLTexInfo<target_enum>                    \
        , public detail::ObjectHandleTypeInfo<Raw##tex_name, MutT> \
    {                                                              \
    public:                                                        \
        using RawTextureHandle<MutT>::RawTextureHandle;            \
    };                                                             \
                                                                   \
static_assert(detail::gl_texture_object<Raw##tex_name<GLConst>>);  \
static_assert(detail::gl_texture_object<Raw##tex_name<GLMutable>>);



GENERATE_TEXTURE_CLASSES(Texture2D,      gl::GL_TEXTURE_2D)
GENERATE_TEXTURE_CLASSES(Texture2DArray, gl::GL_TEXTURE_2D_ARRAY)
GENERATE_TEXTURE_CLASSES(Texture2DMS,    gl::GL_TEXTURE_2D_MULTISAMPLE)
GENERATE_TEXTURE_CLASSES(Cubemap,        gl::GL_TEXTURE_CUBE_MAP)
GENERATE_TEXTURE_CLASSES(CubemapArray,   gl::GL_TEXTURE_CUBE_MAP_ARRAY)

#undef GENERATE_TEXTURE_CLASSES




class TextureData;
class CubemapData;

// FIXME: Awkward Glue between data and gl.
void attach_data_to_texture(BoundTexture2D<GLMutable>& tex,
    const TextureData& data, GLenum internal_format);

void attach_data_to_texture(BoundTexture2D<GLMutable>& tex,
    const TextureData& data, GLenum internal_format, GLenum format);


void attach_data_to_cubemap(BoundCubemap<GLMutable>& cube,
    const CubemapData& data, GLenum internal_format);

void attach_data_to_cubemap(BoundCubemap<GLMutable>& cube,
    const CubemapData& data, GLenum internal_format, GLenum format);




} // namespace josh
