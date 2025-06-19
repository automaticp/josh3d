#pragma once
#include "CommonConcepts.hpp"
#include "CommonMacros.hpp"
#include "ContainerUtils.hpp"
#include "EnumUtils.hpp"
#include "GLAPI.hpp"
#include "GLAPITargets.hpp"
#include "GLScalars.hpp"
#include "Semantics.hpp"
#include "detail/GLAPIGet.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>


namespace josh {


/*
SECTION: Binding.
*/

/*
A token returned from binding functions that is intended to be consumed by functions
that depend on bound state. This is a way to make implicit bound state *explicit*,
and hopefully prevent numerous binding-related bugs.
*/
template<auto B>
class BindToken;

/*
Non-indexed binding slots.
*/
enum class Binding : GLuint
{
    ArrayBuffer            = GLuint(gl::GL_ARRAY_BUFFER_BINDING),
    AtomicCounterBuffer    = GLuint(gl::GL_ATOMIC_COUNTER_BUFFER_BINDING),
    CopyReadBuffer         = GLuint(gl::GL_COPY_READ_BUFFER_BINDING),
    CopyWriteBuffer        = GLuint(gl::GL_COPY_WRITE_BUFFER_BINDING),
    DispatchIndirectBuffer = GLuint(gl::GL_DISPATCH_INDIRECT_BUFFER_BINDING),
    DrawIndirectBuffer     = GLuint(gl::GL_DRAW_INDIRECT_BUFFER_BINDING),
    ElementArrayBuffer     = GLuint(gl::GL_ELEMENT_ARRAY_BUFFER_BINDING),
    ParameterBuffer        = GLuint(gl::GL_PARAMETER_BUFFER_BINDING),
    PixelPackBuffer        = GLuint(gl::GL_PIXEL_PACK_BUFFER_BINDING),
    PixelUnpackBuffer      = GLuint(gl::GL_PIXEL_UNPACK_BUFFER_BINDING),
    QueryBuffer            = GLuint(gl::GL_QUERY_BUFFER_BINDING),
    ShaderStorageBuffer    = GLuint(gl::GL_SHADER_STORAGE_BUFFER_BINDING),
    TextureBuffer          = GLuint(gl::GL_TEXTURE_BUFFER_BINDING),
    UniformBuffer          = GLuint(gl::GL_UNIFORM_BUFFER_BINDING),
    VertexArray            = GLuint(gl::GL_VERTEX_ARRAY_BINDING),
    DrawFramebuffer        = GLuint(gl::GL_DRAW_FRAMEBUFFER_BINDING),
    ReadFramebuffer        = GLuint(gl::GL_READ_FRAMEBUFFER_BINDING),
    TransformFeedback      = GLuint(gl::GL_TRANSFORM_FEEDBACK_BINDING),
    Renderbuffer           = GLuint(gl::GL_RENDERBUFFER_BINDING),
    BufferTexture          = GLuint(gl::GL_TEXTURE_BINDING_BUFFER),
    Texture1D              = GLuint(gl::GL_TEXTURE_BINDING_1D),
    Texture1DArray         = GLuint(gl::GL_TEXTURE_BINDING_1D_ARRAY),
    TextureRectangle       = GLuint(gl::GL_TEXTURE_BINDING_RECTANGLE),
    Texture2D              = GLuint(gl::GL_TEXTURE_BINDING_2D),
    Texture2DArray         = GLuint(gl::GL_TEXTURE_BINDING_2D_ARRAY),
    Texture2DMS            = GLuint(gl::GL_TEXTURE_BINDING_2D_MULTISAMPLE),
    Texture2DMSArray       = GLuint(gl::GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY),
    Texture3D              = GLuint(gl::GL_TEXTURE_BINDING_3D),
    Cubemap                = GLuint(gl::GL_TEXTURE_BINDING_CUBE_MAP),
    CubemapArray           = GLuint(gl::GL_TEXTURE_BINDING_CUBE_MAP_ARRAY),
    Program                = GLuint(gl::GL_CURRENT_PROGRAM),
    ProgramPipeline        = GLuint(gl::GL_PROGRAM_PIPELINE_BINDING),
    // Sampler                 = GLuint(gl::GL_SAMPLER_BINDING),               // Indexed only
    // TransformFeedbackBuffer = GLuint(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING), // Indexed only
};
JOSH3D_DEFINE_ENUM_EXTRAS(Binding,
    ArrayBuffer,
    AtomicCounterBuffer,
    CopyReadBuffer,
    CopyWriteBuffer,
    DispatchIndirectBuffer,
    DrawIndirectBuffer,
    ElementArrayBuffer,
    ParameterBuffer,
    PixelPackBuffer,
    PixelUnpackBuffer,
    QueryBuffer,
    ShaderStorageBuffer,
    TextureBuffer,
    UniformBuffer,
    VertexArray,
    DrawFramebuffer,
    ReadFramebuffer,
    TransformFeedback,
    Renderbuffer,
    BufferTexture,
    Texture1D,
    Texture1DArray,
    TextureRectangle,
    Texture2D,
    Texture2DArray,
    Texture2DMS,
    Texture2DMSArray,
    Texture3D,
    Cubemap,
    CubemapArray,
    Program,
    ProgramPipeline);

/*
Indexed binding slots.
*/
enum class BindingIndexed : GLuint
{
    TransformFeedbackBuffer = GLuint(gl::GL_TRANSFORM_FEEDBACK_BUFFER_BINDING),
    UniformBuffer           = GLuint(gl::GL_UNIFORM_BUFFER_BINDING),
    ShaderStorageBuffer     = GLuint(gl::GL_SHADER_STORAGE_BUFFER_BINDING),
    AtomicCounterBuffer     = GLuint(gl::GL_ATOMIC_COUNTER_BUFFER_BINDING),
    ImageUnit               = GLuint(gl::GL_IMAGE_BINDING_NAME),
    BufferTexture           = GLuint(gl::GL_TEXTURE_BINDING_BUFFER),
    Texture1D               = GLuint(gl::GL_TEXTURE_BINDING_1D),
    Texture1DArray          = GLuint(gl::GL_TEXTURE_BINDING_1D_ARRAY),
    TextureRectangle        = GLuint(gl::GL_TEXTURE_BINDING_RECTANGLE),
    Texture2D               = GLuint(gl::GL_TEXTURE_BINDING_2D),
    Texture2DArray          = GLuint(gl::GL_TEXTURE_BINDING_2D_ARRAY),
    Texture2DMS             = GLuint(gl::GL_TEXTURE_BINDING_2D_MULTISAMPLE),
    Texture2DMSArray        = GLuint(gl::GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY),
    Texture3D               = GLuint(gl::GL_TEXTURE_BINDING_3D),
    Cubemap                 = GLuint(gl::GL_TEXTURE_BINDING_CUBE_MAP),
    CubemapArray            = GLuint(gl::GL_TEXTURE_BINDING_CUBE_MAP_ARRAY),
    Sampler                 = GLuint(gl::GL_SAMPLER_BINDING),
};
JOSH3D_DEFINE_ENUM_EXTRAS(BindingIndexed,
    TransformFeedbackBuffer,
    UniformBuffer,
    ShaderStorageBuffer,
    AtomicCounterBuffer,
    ImageUnit,
    BufferTexture,
    Texture1D,
    Texture1DArray,
    TextureRectangle,
    Texture2D,
    Texture2DArray,
    Texture2DMS,
    Texture2DMSArray,
    Texture3D,
    Cubemap,
    CubemapArray,
    Sampler);

// TODO: Rename for brevity.
using BindingI = BindingIndexed;

namespace glapi {

/*
Wraps `glGetIntegerv` with `pname = binding`.

Returns the id (name) currently bound to the specified binding slot.
*/
inline auto get_bound_id(Binding binding)
    -> GLuint
{
    return detail::get_integer(enum_cast<GLenum>(binding));
}

/*
Wraps `glGetIntegeri_v` with `pname = binding`.

Returns the id (name) currently bound to the specified indexed binding slot.
*/
inline auto get_bound_id(BindingI binding, GLuint index)
    -> GLuint
{
    return detail::get_integer_indexed(enum_cast<GLenum>(binding), index);
}


/*
NOTE: These are general functions and may fail to represent a full set of binding options,
or expose bindings that are "obsoleted" by DSA. They are usually only used in the implementation.
Prefer to use the per-object binding functions for binding, and `make_availible()` for cross-context
visibility updates.

NOTE: Specialized below, after BindToken.
*/

template<Binding  B> auto bind_to_context(GLuint id) -> BindToken<B>;
template<BindingI B> auto bind_to_context(GLuint index, GLuint id) -> BindToken<B>;
template<Binding  B> void unbind_from_context();
template<BindingI B> void unbind_from_context(GLuint index);

/*
For cross-conext visibility updates.
*/
template<Binding B>
inline void make_available(GLuint id)
{
    bind_to_context<B>(id);
    unbind_from_context<B>();
}


} // namespace glapi


template<typename B>
concept binding_like = any_of<B, Binding, BindingI>;

template<Binding B>
class BindToken<B>
{
public:
    using binding_type = Binding;
    static constexpr binding_type binding    = B;
    static constexpr bool         is_indexed = false;

    auto id()     const noexcept -> GLuint { return id_; }
    void unbind() const { glapi::unbind_from_context<B>(); }

    // NOTE: Should only be called in the implementations of the binding functions.
    static auto from_id(GLuint id) noexcept -> BindToken { return { id }; }

private:
    GLuint id_;
    BindToken(GLuint id) noexcept : id_{ id } {}
};

template<BindingI B>
class BindToken<B>
{
public:
    using binding_type = BindingI;
    static constexpr binding_type binding    = B;
    static constexpr bool         is_indexed = true;

    auto id()     const noexcept -> GLuint { return id_; }
    auto index()  const noexcept -> GLuint { return index_; }
    void unbind() const { glapi::unbind_from_context<B>(index_); }

    // NOTE: Should only be called in the implementations of the binding functions.
    static auto from_index_and_id(GLuint index, GLuint id) noexcept -> BindToken { return { index, id }; }

private:
    GLuint index_;
    GLuint id_;
    BindToken(GLuint index, GLuint id) noexcept : index_{ index }, id_{ id } {}
};


namespace glapi {

#define _JOSH3D_DEFINE_BIND(B, BindFunc, ...)          \
    template<>                                         \
    inline auto bind_to_context<Binding::B>(GLuint id) \
        -> BindToken<Binding::B>                       \
    {                                                  \
        gl::BindFunc(__VA_ARGS__ id);                  \
        return BindToken<Binding::B>::from_id(id);     \
    }                                                  \
    template<>                                         \
    inline void unbind_from_context<Binding::B>()      \
    {                                                  \
        gl::BindFunc(__VA_ARGS__ 0);                   \
    }
#define JOSH3D_DEFINE_BIND0(B, BindFunc)      _JOSH3D_DEFINE_BIND(B, BindFunc)
#define JOSH3D_DEFINE_BIND1(B, BindFunc, Arg) _JOSH3D_DEFINE_BIND(B, BindFunc, JOSH3D_SINGLE_ARG(Arg,))

/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(ArrayBuffer,            glBindBuffer,            gl::GL_ARRAY_BUFFER                )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(AtomicCounterBuffer,    glBindBuffer,            gl::GL_ATOMIC_COUNTER_BUFFER       )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(CopyReadBuffer,         glBindBuffer,            gl::GL_COPY_READ_BUFFER            )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(CopyWriteBuffer,        glBindBuffer,            gl::GL_COPY_WRITE_BUFFER           )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(DispatchIndirectBuffer, glBindBuffer,            gl::GL_DISPATCH_INDIRECT_BUFFER    )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(DrawIndirectBuffer,     glBindBuffer,            gl::GL_DRAW_INDIRECT_BUFFER        )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(ElementArrayBuffer,     glBindBuffer,            gl::GL_ELEMENT_ARRAY_BUFFER        )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(ParameterBuffer,        glBindBuffer,            gl::GL_PARAMETER_BUFFER            )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(PixelPackBuffer,        glBindBuffer,            gl::GL_PIXEL_PACK_BUFFER           )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(PixelUnpackBuffer,      glBindBuffer,            gl::GL_PIXEL_UNPACK_BUFFER         )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(QueryBuffer,            glBindBuffer,            gl::GL_QUERY_BUFFER                )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(ShaderStorageBuffer,    glBindBuffer,            gl::GL_SHADER_STORAGE_BUFFER       )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(TextureBuffer,          glBindBuffer,            gl::GL_TEXTURE_BUFFER              )
/* Wraps `glBindBuffer`.            */ JOSH3D_DEFINE_BIND1(UniformBuffer,          glBindBuffer,            gl::GL_UNIFORM_BUFFER              )
/* Wraps `glBindVertexArray`.       */ JOSH3D_DEFINE_BIND0(VertexArray,            glBindVertexArray                                           )
/* Wraps `glBindFramebuffer`.       */ JOSH3D_DEFINE_BIND1(DrawFramebuffer,        glBindFramebuffer,       gl::GL_DRAW_FRAMEBUFFER            )
/* Wraps `glBindFramebuffer`.       */ JOSH3D_DEFINE_BIND1(ReadFramebuffer,        glBindFramebuffer,       gl::GL_READ_FRAMEBUFFER            )
/* Wraps `glBindTransformFeedback`. */ JOSH3D_DEFINE_BIND1(TransformFeedback,      glBindTransformFeedback, gl::GL_TRANSFORM_FEEDBACK          )
/* Wraps `glBindRenderbuffer`.      */ JOSH3D_DEFINE_BIND1(Renderbuffer,           glBindRenderbuffer,      gl::GL_RENDERBUFFER                )
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(BufferTexture,          glBindTexture,           gl::GL_TEXTURE_BUFFER              )
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(Texture1D,              glBindTexture,           gl::GL_TEXTURE_1D                  )
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(Texture1DArray,         glBindTexture,           gl::GL_TEXTURE_1D_ARRAY            )
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(TextureRectangle,       glBindTexture,           gl::GL_TEXTURE_RECTANGLE           )
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(Texture2D,              glBindTexture,           gl::GL_TEXTURE_2D                  )
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(Texture2DArray,         glBindTexture,           gl::GL_TEXTURE_2D_ARRAY            )
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(Texture2DMS,            glBindTexture,           gl::GL_TEXTURE_2D_MULTISAMPLE      )
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(Texture2DMSArray,       glBindTexture,           gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(Texture3D,              glBindTexture,           gl::GL_TEXTURE_3D                  )
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(Cubemap,                glBindTexture,           gl::GL_TEXTURE_CUBE_MAP            )
/* Wraps `glBindTexture`.           */ JOSH3D_DEFINE_BIND1(CubemapArray,           glBindTexture,           gl::GL_TEXTURE_CUBE_MAP_ARRAY      )
/* Wraps `glUseProgram`.            */ JOSH3D_DEFINE_BIND0(Program,                glUseProgram                                                )
/* Wraps `glBindProgramPipeline`.   */ JOSH3D_DEFINE_BIND0(ProgramPipeline,        glBindProgramPipeline                                       )

#undef _JOSH3D_DEFINE_BIND
#undef JOSH3D_DEFINE_BIND0
#undef JOSH3D_DEFINE_BIND1

#define _JOSH3D_DEFINE_BIND(B, BindFunc, ...)                                 \
    template<>                                                                \
    inline auto bind_to_context<BindingI::B>(GLuint index, GLuint id)         \
        -> BindToken<BindingI::B>                                             \
    {                                                                         \
        gl::BindFunc(__VA_ARGS__ index, id);                                  \
        return BindToken<BindingI::B>::from_index_and_id(index, id);          \
    }                                                                         \
    template<>                                                                \
    inline void unbind_from_context<BindingI::B>(GLuint index)                \
    {                                                                         \
        gl::BindFunc(__VA_ARGS__ index, 0);                                   \
    }
#define JOSH3D_DEFINE_BIND0(B, BindFunc)      _JOSH3D_DEFINE_BIND(B, BindFunc)
#define JOSH3D_DEFINE_BIND1(B, BindFunc, Arg) _JOSH3D_DEFINE_BIND(B, BindFunc, JOSH3D_SINGLE_ARG(Arg,))

/* Wraps `glBindBufferBase`.  */ JOSH3D_DEFINE_BIND1(ShaderStorageBuffer,     glBindBufferBase,    gl::GL_SHADER_STORAGE_BUFFER    )
/* Wraps `glBindBufferBase`.  */ JOSH3D_DEFINE_BIND1(UniformBuffer,           glBindBufferBase,    gl::GL_UNIFORM_BUFFER           )
/* Wraps `glBindBufferBase`.  */ JOSH3D_DEFINE_BIND1(TransformFeedbackBuffer, glBindBufferBase,    gl::GL_TRANSFORM_FEEDBACK_BUFFER)
/* Wraps `glBindBufferBase`.  */ JOSH3D_DEFINE_BIND1(AtomicCounterBuffer,     glBindBufferBase,    gl::GL_ATOMIC_COUNTER_BUFFER    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(BufferTexture,           glBindTextureUnit                                    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(Texture1D,               glBindTextureUnit                                    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(Texture1DArray,          glBindTextureUnit                                    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(TextureRectangle,        glBindTextureUnit                                    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(Texture2D,               glBindTextureUnit                                    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(Texture2DArray,          glBindTextureUnit                                    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(Texture2DMS,             glBindTextureUnit                                    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(Texture2DMSArray,        glBindTextureUnit                                    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(Texture3D,               glBindTextureUnit                                    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(Cubemap,                 glBindTextureUnit                                    )
/* Wraps `glBindTextureUnit`. */ JOSH3D_DEFINE_BIND0(CubemapArray,            glBindTextureUnit                                    )
/* Wraps `glBindSampler`.     */ JOSH3D_DEFINE_BIND0(Sampler,                 glBindSampler                                        )

/*
NOTE: There's no bind_to_context() for ImageUnit since it would likely be invalid.
Use Texture-specific calls that bind with a correct format, access, layer, etc.
*/
template<>
inline auto bind_to_context<BindingI::ImageUnit>(GLuint index, GLuint id)
    -> BindToken<BindingI::ImageUnit> = delete;

template<>
inline void unbind_from_context<BindingI::ImageUnit>(GLuint index)
{
    gl::glBindImageTexture(index, 0, 0, gl::GL_FALSE, 0, gl::GL_READ_ONLY, gl::GL_R8);
}

#undef _JOSH3D_DEFINE_BIND
#undef JOSH3D_DEFINE_BIND0
#undef JOSH3D_DEFINE_BIND1


/*
Backwards compatibility, consider deprecating.
*/
inline void unbind_sampler_from_unit(GLuint index)
{
    unbind_from_context<BindingI::Sampler>(index);
}

/*
Wraps `glBindSamplers`.
*/
inline void bind_sampler_units(std::span<const GLuint> samplers, GLuint first = 0)
{
    gl::glBindSamplers(first, GLsizei(samplers.size()), samplers.data());
}

/*
Wraps `glBindTextures()`.
*/
inline void bind_texture_units(std::span<const GLuint> textures, GLuint first = 0)
{
    gl::glBindTextures(first, GLsizei(textures.size()), textures.data());
}

} // namespace glapi


/*
An RAII guard that automatically unbinds at the end of scope.
*/
template<binding_like auto B>
class BindGuard
    : Immovable<BindGuard<B>>
{
public:
    using token_type   = BindToken<B>;
    using binding_type = token_type::binding_type;
    static constexpr bool is_indexed = token_type::is_indexed;

    BindGuard(BindToken<B> token) : token_{ token } {}

    operator BindToken<B>() const noexcept { return token_; }
    BindToken<B> token()    const noexcept { return token_; }

    auto id()    const noexcept -> GLuint { return token_.id(); }
    auto index() const noexcept -> GLuint requires is_indexed { return token_.index(); }

    ~BindGuard() noexcept { token_.unbind(); }

private:
    BindToken<B> token_;
};

/*
An RAII guard that automatically unbinds multiple bindings at the end of scope.
*/
template<binding_like auto ...B>
class MultibindGuard
    : Immovable<MultibindGuard<B...>>
{
private:
    using tokens_type = std::tuple<BindToken<B>...>;
public:
    template<size_t Idx> using token_type   = std::tuple_element_t<Idx, tokens_type>;
    template<size_t Idx> using binding_type = token_type<Idx>::binding_type;
    template<size_t Idx> static constexpr bool is_indexed = token_type<Idx>::is_indexed;

    static constexpr size_t num_guarded = std::tuple_size<tokens_type>();

    MultibindGuard(BindToken<B>... tokens) : tokens_{ tokens... } {}

    template<size_t Idx> auto token() const noexcept { return std::get<Idx>(tokens_); }

    template<size_t Idx> auto id()    const noexcept -> GLuint { return token<Idx>().id(); }
    template<size_t Idx> auto index() const noexcept -> GLuint requires is_indexed<Idx> { return token<Idx>().index(); }

    ~MultibindGuard() noexcept
    {
        EXPAND(I, num_guarded, this)
        {
            (token<I>().unbind(), ...);
        };
    }

private:
    tokens_type tokens_;
};


/*
Returns primary `Binding` for the specified `target`.
*/
constexpr auto target_binding(TextureTarget target) noexcept
    -> Binding
{
    switch (target)
    {
        using enum TextureTarget;
        case Texture1D:        return Binding::Texture1D;
        case Texture1DArray:   return Binding::Texture1DArray;
        case Texture2D:        return Binding::Texture2D;
        case Texture2DArray:   return Binding::Texture2DArray;
        case Texture2DMS:      return Binding::Texture2DMS;
        case Texture2DMSArray: return Binding::Texture2DMSArray;
        case Texture3D:        return Binding::Texture3D;
        case Cubemap:          return Binding::Cubemap;
        case CubemapArray:     return Binding::CubemapArray;
        case TextureRectangle: return Binding::TextureRectangle;
        case TextureBuffer:    return Binding::TextureBuffer;
    }
    assert(false);
    return {};
}

constexpr auto target_binding(BufferTarget target) noexcept
    -> Binding
{
    switch (target)
    {
        using enum BufferTarget;
        case DispatchIndirect: return Binding::DispatchIndirectBuffer;
        case DrawIndirect:     return Binding::DrawIndirectBuffer;
        case Parameter:        return Binding::ParameterBuffer;
        case PixelPack:        return Binding::PixelPackBuffer;
        case PixelUnpack:      return Binding::PixelUnpackBuffer;
    }
    assert(false);
    return {};
}

constexpr auto target_binding_indexed(BufferTargetI target) noexcept
    -> BindingI
{
    switch (target)
    {
        using enum BufferTargetI;
        case ShaderStorage:     return BindingI::ShaderStorageBuffer;
        case Uniform:           return BindingI::UniformBuffer;
        case TransformFeedback: return BindingI::TransformFeedbackBuffer;
        case AtomicCounter:     return BindingI::AtomicCounterBuffer;
    }
    assert(false);
    return {};
}

constexpr auto target_binding_indexed(TextureTarget target) noexcept
    -> BindingI
{
    switch (target)
    {
        using enum TextureTarget;
        case Texture1D:        return BindingI::Texture1D;
        case Texture1DArray:   return BindingI::Texture1DArray;
        case Texture2D:        return BindingI::Texture2D;
        case Texture2DArray:   return BindingI::Texture2DArray;
        case Texture2DMS:      return BindingI::Texture2DMS;
        case Texture2DMSArray: return BindingI::Texture2DMSArray;
        case Texture3D:        return BindingI::Texture3D;
        case Cubemap:          return BindingI::Cubemap;
        case CubemapArray:     return BindingI::CubemapArray;
        case TextureRectangle: return BindingI::TextureRectangle;
        case TextureBuffer:    return BindingI::BufferTexture; // This mismatch is not my fault.
    }
    assert(false);
    return {};
}

// TODO: Query targets?


} // namespace josh
