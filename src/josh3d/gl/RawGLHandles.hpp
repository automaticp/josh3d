#pragma once
#include "GLMutability.hpp"
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "GLScalars.hpp"
#include <concepts>
#include <utility>




namespace josh {


/*
The base class of all OpenGL handles. Represents a fully opaque
handle that has no knowledge of its type or allocation method.

Models a raw `void*` or `const void*` depending on mutability.
*/
template<mutability_tag MutT>
class RawGLHandle : public MutT {
protected:
    // Maybe I could make something like: enum class GLHandle : GLuint {}; ?
    GLuint id_; // Should this be private?
    GLuint reset_id(GLuint new_id = GLuint{ 0 }) noexcept {
        GLuint old_id{ id_ };
        id_ = new_id;
        return old_id;
    }
    ~RawGLHandle() = default;
public:
    friend RawGLHandle<OppositeGLMutability<MutT>>;

    explicit RawGLHandle(GLuint id) : id_{ id } {}

    RawGLHandle(const RawGLHandle&)     = default;
    RawGLHandle(RawGLHandle&&) noexcept = default;
    RawGLHandle& operator=(const RawGLHandle&)     = default;
    RawGLHandle& operator=(RawGLHandle&&) noexcept = default;

    // GLMutable -> GLConst   conversions are permitted

    RawGLHandle(const RawGLHandle<GLMutable>& other) noexcept
        requires gl_const<MutT>
        : id_{ other.id_ }
    {}

    RawGLHandle& operator=(const RawGLHandle<GLMutable>& other) noexcept
        requires gl_const<MutT>
    {
        id_ = other.id_;
        return *this;
    }

    RawGLHandle(RawGLHandle<GLMutable>&& other) noexcept
        requires gl_const<MutT>
        : id_{ other.id_ }
    {}

    RawGLHandle& operator=(RawGLHandle<GLMutable>&& other) noexcept
        requires gl_const<MutT>
    {
        id_ = other.id_;
        return *this;
    }

    // GLConst   -> GLMutable conversions are forbidden

    RawGLHandle(const RawGLHandle<GLConst>&) noexcept
        requires gl_mutable<MutT> = delete;

    RawGLHandle& operator=(const RawGLHandle<GLConst>&) noexcept
        requires gl_mutable<MutT> = delete;

    RawGLHandle(RawGLHandle<GLConst>&&) noexcept
        requires gl_mutable<MutT> = delete;

    RawGLHandle& operator=(RawGLHandle<GLConst>&&) noexcept
        requires gl_mutable<MutT> = delete;


    GLuint id() const noexcept { return id_; }
    operator GLuint() const noexcept { return id_; }
};



// Since constructibility might not be inherited,
// it makes sense to check this for both kind-handles and object-handles,
// even if they derive from the RawGLHandle.
template<typename RawT>
concept has_basic_raw_handle_semantics = requires(RawT raw) {
    // Constructible from GLuint.
    RawT{ GLuint{ 42 } };
    // Can return or be converted to the object id.
    { raw.id() }                 -> same_as_remove_cvref<GLuint>;
    { std::as_const(raw).id() }  -> same_as_remove_cvref<GLuint>;
    { static_cast<GLuint>(raw) } -> same_as_remove_cvref<GLuint>;
};


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
    requires derived_from_any_of<RawKindH, RawGLHandle<GLConst>, RawGLHandle<GLMutable>>;
    requires has_basic_raw_handle_semantics<RawKindH>;
    requires specifies_mutability<RawKindH>;
    typename RawKindH::kind_handle_type;
    requires std::same_as<typename RawKindH::kind_handle_type, RawKindH>;
};



#define GENERATE_KIND_HANDLE(kind)                                 \
    template<mutability_tag MutT>                                  \
    struct Raw##kind##Handle                                       \
        : RawGLHandle<MutT>                                        \
        , detail::KindHandleTypeInfo<Raw##kind##Handle, MutT>      \
    {                                                              \
        using RawGLHandle<MutT>::RawGLHandle;                      \
    };                                                             \
                                                                   \
    static_assert(raw_gl_kind_handle<Raw##kind##Handle<GLConst>>); \
    static_assert(raw_gl_kind_handle<Raw##kind##Handle<GLMutable>>);


GENERATE_KIND_HANDLE(Texture)
GENERATE_KIND_HANDLE(Buffer)
GENERATE_KIND_HANDLE(VertexArray)
GENERATE_KIND_HANDLE(Framebuffer)
GENERATE_KIND_HANDLE(Renderbuffer)
GENERATE_KIND_HANDLE(Shader)
GENERATE_KIND_HANDLE(ShaderProgram)

#undef GENERATE_KIND_HANDLE



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
    // requires std::derived_from<RawObjT, GLObjectType<RawObjT::template object_type_template>>; // Huh?
    requires specifies_mutability<RawObjH>;
    requires std::same_as<RawObjH, typename RawObjH::object_handle_type>;
    requires std::same_as<RawObjH,
        typename RawObjH::template object_handle_type_template<typename RawObjH::mutability_type>>;
    requires has_basic_raw_handle_semantics<RawObjH>;
};







} // namespace josh
