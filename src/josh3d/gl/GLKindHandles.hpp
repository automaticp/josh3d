#pragma once
#include "detail/RawGLHandle.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "GLMutability.hpp"
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLScalars.hpp"
#include <concepts>
#include <utility>




namespace josh {
namespace detail {


template<
    template<typename/* clangd crashes if I put a concept here */> typename TemplateCRTP,
    mutability_tag MutT
>
struct KindHandleTypeInfo {
    using kind_handle_type          = TemplateCRTP<MutT>;
    using kind_handle_const_type    = TemplateCRTP<GLConst>;
    using kind_handle_mutable_type  = TemplateCRTP<GLMutable>;

    template<mutability_tag MutU>
    using kind_handle_type_template = TemplateCRTP<MutU>;
};


} // namespace detail


/*
Handle types to disambiguate allocators for each object kind.

Models a raw "kind" pointer, as if it was:
    `TextureKind*` or `const TextureKind*` depending on mutability.

This carries no information about the object's `target`, and consequently,
does not fully describe a "type" of an OpenGL object.

Knowing the object kind allows you to request the OpenGL handle
through correct API calls (glGenTextures and glDeleteTextures for example above).

Interestingly, certain object kinds (buffers, especially) allow rebinding
between different target types, while preserving the underlying handle and storage.
*/
template<typename RawKindH>
concept raw_gl_kind_handle = requires {
    requires derived_from_any_of<RawKindH, detail::RawGLHandle<GLConst>, detail::RawGLHandle<GLMutable>>;
    requires detail::has_basic_raw_handle_semantics<RawKindH>;
    requires specifies_mutability<RawKindH>;
    typename RawKindH::kind_handle_type;
    requires std::same_as<typename RawKindH::kind_handle_type, RawKindH>;
};



#define JOSH3D_GENERATE_KIND_HANDLE(kind)                                             \
    template<mutability_tag MutT>                                                     \
    struct Raw##kind##Handle                                                          \
        : detail::RawGLHandle<MutT>                                                   \
        , detail::KindHandleTypeInfo<Raw##kind##Handle, MutT>                         \
    {                                                                                 \
        JOSH3D_MAGIC_CONSTRUCTORS(Raw##kind##Handle, MutT, detail::RawGLHandle<MutT>) \
    };                                                                                \
                                                                                      \
    static_assert(raw_gl_kind_handle<Raw##kind##Handle<GLConst>>);                    \
    static_assert(raw_gl_kind_handle<Raw##kind##Handle<GLMutable>>);


JOSH3D_GENERATE_KIND_HANDLE(Texture)
JOSH3D_GENERATE_KIND_HANDLE(Buffer)
JOSH3D_GENERATE_KIND_HANDLE(VertexArray)
JOSH3D_GENERATE_KIND_HANDLE(Framebuffer)
JOSH3D_GENERATE_KIND_HANDLE(Renderbuffer)
JOSH3D_GENERATE_KIND_HANDLE(Shader)
JOSH3D_GENERATE_KIND_HANDLE(ShaderProgram)

#undef JOSH3D_GENERATE_KIND_HANDLE



namespace detail{

/*
Allows you to "reflect" on the object type with stripped mutability.
Can go from GLMutable to GLConst through this:
    RawTexture2D<GLConst>::object_handle_type_template<GLMutable> -> RawTexture2D<GLMutable>

Also there's probably a cleaner way.
*/
template<
    template<typename/* clangd crashes if I put a concept here */> typename TemplateCRTP,
    mutability_tag MutT
>
struct ObjectHandleTypeInfo {
    using object_handle_type          = TemplateCRTP<MutT>;
    using object_handle_const_type    = TemplateCRTP<GLConst>;
    using object_handle_mutable_type  = TemplateCRTP<GLMutable>;

    template<mutability_tag MutU>
    using object_handle_type_template = TemplateCRTP<MutU>;
    // I'm still surprised this actually works.
    // TMP is truly write-only...
};

} // namespace detail




/*
Raw object types impose `target` semantics on the OpenGL object kinds.

Meaning, RawTexture2D object type binds and behaves like GL_TEXTURE_2D,
while RawCubemap - like GL_TEXTURE_CUBE_MAP. At the same time, both
of them belong to the same object "kind", so the underlying handle type
is the same for both - RawTextureHandle.
*/
template<typename RawObjH>
concept raw_gl_object_handle = requires {
    requires std::derived_from<RawObjH, typename RawObjH::kind_handle_type>;
    requires raw_gl_kind_handle<typename RawObjH::kind_handle_type>;
    // Reflects on self through GLObjectType::object_type_template.
    requires specifies_mutability<RawObjH>;
    requires std::same_as<RawObjH, typename RawObjH::object_handle_type>;
    requires std::same_as<RawObjH,
        typename RawObjH::template object_handle_type_template<typename RawObjH::mutability_type>>;
    requires detail::has_basic_raw_handle_semantics<RawObjH>;
};







} // namespace josh
