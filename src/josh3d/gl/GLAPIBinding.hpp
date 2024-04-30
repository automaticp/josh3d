#pragma once
#include "CommonConcepts.hpp"
#include "GLAPI.hpp"
#include "GLScalars.hpp"
#include <concepts>


namespace josh {



// Queriable with glGetIntegerv
enum class Binding : GLuint {

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

    // Sampler                = GLuint(gl::GL_SAMPLER_BINDING), // Indexed only

    Program                = GLuint(gl::GL_CURRENT_PROGRAM),
    ProgramPipeline        = GLuint(gl::GL_PROGRAM_PIPELINE_BINDING),

    // GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, // Indexed only?
};


// Queriable with glGetIntegeri_v
enum class BindingIndexed : GLuint {

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


template<Binding B> inline void bind_to_context(GLuint id) noexcept;

template<> inline void bind_to_context<Binding::ArrayBuffer>           (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_ARRAY_BUFFER,                 id); }
template<> inline void bind_to_context<Binding::AtomicCounterBuffer>   (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_ATOMIC_COUNTER_BUFFER,        id); }
template<> inline void bind_to_context<Binding::CopyReadBuffer>        (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_COPY_READ_BUFFER,             id); }
template<> inline void bind_to_context<Binding::CopyWriteBuffer>       (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_COPY_WRITE_BUFFER,            id); }
template<> inline void bind_to_context<Binding::DispatchIndirectBuffer>(GLuint id) noexcept { gl::glBindBuffer           (gl::GL_DISPATCH_INDIRECT_BUFFER,     id); }
template<> inline void bind_to_context<Binding::DrawIndirectBuffer>    (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_DRAW_INDIRECT_BUFFER,         id); }
template<> inline void bind_to_context<Binding::ElementArrayBuffer>    (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_ELEMENT_ARRAY_BUFFER,         id); }
template<> inline void bind_to_context<Binding::ParameterBuffer>       (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_PARAMETER_BUFFER,             id); }
template<> inline void bind_to_context<Binding::PixelPackBuffer>       (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_PIXEL_PACK_BUFFER,            id); }
template<> inline void bind_to_context<Binding::PixelUnpackBuffer>     (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_PIXEL_UNPACK_BUFFER,          id); }
template<> inline void bind_to_context<Binding::QueryBuffer>           (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_QUERY_BUFFER,                 id); }
template<> inline void bind_to_context<Binding::ShaderStorageBuffer>   (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_SHADER_STORAGE_BUFFER,        id); }
template<> inline void bind_to_context<Binding::TextureBuffer>         (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_TEXTURE_BUFFER,               id); }
template<> inline void bind_to_context<Binding::UniformBuffer>         (GLuint id) noexcept { gl::glBindBuffer           (gl::GL_UNIFORM_BUFFER,               id); }
template<> inline void bind_to_context<Binding::VertexArray>           (GLuint id) noexcept { gl::glBindVertexArray      (                                     id); }
template<> inline void bind_to_context<Binding::DrawFramebuffer>       (GLuint id) noexcept { gl::glBindFramebuffer      (gl::GL_DRAW_FRAMEBUFFER,             id); }
template<> inline void bind_to_context<Binding::ReadFramebuffer>       (GLuint id) noexcept { gl::glBindFramebuffer      (gl::GL_READ_FRAMEBUFFER,             id); }
template<> inline void bind_to_context<Binding::TransformFeedback>     (GLuint id) noexcept { gl::glBindTransformFeedback(gl::GL_TRANSFORM_FEEDBACK,           id); }
template<> inline void bind_to_context<Binding::Renderbuffer>          (GLuint id) noexcept { gl::glBindRenderbuffer     (gl::GL_RENDERBUFFER,                 id); }
template<> inline void bind_to_context<Binding::BufferTexture>         (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_BUFFER,               id); }
template<> inline void bind_to_context<Binding::Texture1D>             (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_1D,                   id); }
template<> inline void bind_to_context<Binding::Texture1DArray>        (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_1D_ARRAY,             id); }
template<> inline void bind_to_context<Binding::TextureRectangle>      (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_RECTANGLE,            id); }
template<> inline void bind_to_context<Binding::Texture2D>             (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D,                   id); }
template<> inline void bind_to_context<Binding::Texture2DArray>        (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D_ARRAY,             id); }
template<> inline void bind_to_context<Binding::Texture2DMS>           (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D_MULTISAMPLE,       id); }
template<> inline void bind_to_context<Binding::Texture2DMSArray>      (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY, id); }
template<> inline void bind_to_context<Binding::Texture3D>             (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_3D,                   id); }
template<> inline void bind_to_context<Binding::Cubemap>               (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_CUBE_MAP,             id); }
template<> inline void bind_to_context<Binding::CubemapArray>          (GLuint id) noexcept { gl::glBindTexture          (gl::GL_TEXTURE_CUBE_MAP_ARRAY,       id); }
template<> inline void bind_to_context<Binding::Program>               (GLuint id) noexcept { gl::glUseProgram           (                                     id); }
template<> inline void bind_to_context<Binding::ProgramPipeline>       (GLuint id) noexcept { gl::glBindProgramPipeline  (                                     id); }




template<Binding B> inline void unbind_from_context() noexcept;


template<> inline void unbind_from_context<Binding::ArrayBuffer>           () noexcept { gl::glBindBuffer           (gl::GL_ARRAY_BUFFER,                 0); }
template<> inline void unbind_from_context<Binding::AtomicCounterBuffer>   () noexcept { gl::glBindBuffer           (gl::GL_ATOMIC_COUNTER_BUFFER,        0); }
template<> inline void unbind_from_context<Binding::CopyReadBuffer>        () noexcept { gl::glBindBuffer           (gl::GL_COPY_READ_BUFFER,             0); }
template<> inline void unbind_from_context<Binding::CopyWriteBuffer>       () noexcept { gl::glBindBuffer           (gl::GL_COPY_WRITE_BUFFER,            0); }
template<> inline void unbind_from_context<Binding::DispatchIndirectBuffer>() noexcept { gl::glBindBuffer           (gl::GL_DISPATCH_INDIRECT_BUFFER,     0); }
template<> inline void unbind_from_context<Binding::DrawIndirectBuffer>    () noexcept { gl::glBindBuffer           (gl::GL_DRAW_INDIRECT_BUFFER,         0); }
template<> inline void unbind_from_context<Binding::ElementArrayBuffer>    () noexcept { gl::glBindBuffer           (gl::GL_ELEMENT_ARRAY_BUFFER,         0); }
template<> inline void unbind_from_context<Binding::ParameterBuffer>       () noexcept { gl::glBindBuffer           (gl::GL_PARAMETER_BUFFER,             0); }
template<> inline void unbind_from_context<Binding::PixelPackBuffer>       () noexcept { gl::glBindBuffer           (gl::GL_PIXEL_PACK_BUFFER,            0); }
template<> inline void unbind_from_context<Binding::PixelUnpackBuffer>     () noexcept { gl::glBindBuffer           (gl::GL_PIXEL_UNPACK_BUFFER,          0); }
template<> inline void unbind_from_context<Binding::QueryBuffer>           () noexcept { gl::glBindBuffer           (gl::GL_QUERY_BUFFER,                 0); }
template<> inline void unbind_from_context<Binding::ShaderStorageBuffer>   () noexcept { gl::glBindBuffer           (gl::GL_SHADER_STORAGE_BUFFER,        0); }
template<> inline void unbind_from_context<Binding::TextureBuffer>         () noexcept { gl::glBindBuffer           (gl::GL_TEXTURE_BUFFER,               0); }
template<> inline void unbind_from_context<Binding::UniformBuffer>         () noexcept { gl::glBindBuffer           (gl::GL_UNIFORM_BUFFER,               0); }
template<> inline void unbind_from_context<Binding::VertexArray>           () noexcept { gl::glBindVertexArray      (                                     0); }
template<> inline void unbind_from_context<Binding::DrawFramebuffer>       () noexcept { gl::glBindFramebuffer      (gl::GL_DRAW_FRAMEBUFFER,             0); }
template<> inline void unbind_from_context<Binding::ReadFramebuffer>       () noexcept { gl::glBindFramebuffer      (gl::GL_READ_FRAMEBUFFER,             0); }
template<> inline void unbind_from_context<Binding::TransformFeedback>     () noexcept { gl::glBindTransformFeedback(gl::GL_TRANSFORM_FEEDBACK,           0); }
template<> inline void unbind_from_context<Binding::Renderbuffer>          () noexcept { gl::glBindRenderbuffer     (gl::GL_RENDERBUFFER,                 0); }
template<> inline void unbind_from_context<Binding::BufferTexture>         () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_BUFFER,               0); }
template<> inline void unbind_from_context<Binding::Texture1D>             () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_1D,                   0); }
template<> inline void unbind_from_context<Binding::Texture1DArray>        () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_1D_ARRAY,             0); }
template<> inline void unbind_from_context<Binding::TextureRectangle>      () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_RECTANGLE,            0); }
template<> inline void unbind_from_context<Binding::Texture2D>             () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D,                   0); }
template<> inline void unbind_from_context<Binding::Texture2DArray>        () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D_ARRAY,             0); }
template<> inline void unbind_from_context<Binding::Texture2DMS>           () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D_MULTISAMPLE,       0); }
template<> inline void unbind_from_context<Binding::Texture2DMSArray>      () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0); }
template<> inline void unbind_from_context<Binding::Texture3D>             () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_3D,                   0); }
template<> inline void unbind_from_context<Binding::Cubemap>               () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_CUBE_MAP,             0); }
template<> inline void unbind_from_context<Binding::CubemapArray>          () noexcept { gl::glBindTexture          (gl::GL_TEXTURE_CUBE_MAP_ARRAY,       0); }
template<> inline void unbind_from_context<Binding::Program>               () noexcept { gl::glUseProgram           (                                     0); }
template<> inline void unbind_from_context<Binding::ProgramPipeline>       () noexcept { gl::glBindProgramPipeline  (                                     0); }
// template<> inline void unbind_from_context<Binding::Sampler>()                noexcept {                                                                     }



// TODO: Indexed Unbind.

template<BindingIndexed B> void unbind_indexed_from_context(GLuint index) noexcept;

template<> inline void unbind_indexed_from_context<BindingIndexed::Sampler>(GLuint index)             noexcept { gl::glBindSampler   (                              index, 0); }
template<> inline void unbind_indexed_from_context<BindingIndexed::ShaderStorageBuffer>(GLuint index) noexcept { gl::glBindBufferBase(gl::GL_SHADER_STORAGE_BUFFER, index, 0); }

inline void unbind_sampler_from_unit(GLuint index) noexcept { unbind_indexed_from_context<BindingIndexed::Sampler>(index); }

template<std::convertible_to<GLuint> ...UInt>
void unbind_samplers_from_units(UInt... index) noexcept { (unbind_sampler_from_unit(index), ...); }



// For cross-conext visibility updates.
template<Binding B>
inline void make_available(GLuint id) noexcept {
    bind_to_context<B>(id);
    unbind_from_context<B>();
}






template<Binding B>
class BindToken;


/*
The alternative to this is to have an impl base with protected c-tor and d-tor,
and then do something like:

    template<>
    struct BindToken<Binding::ArrayBuffer>
        : BindTokenImpl<Binding::ArrayBuffer>
    {
        using BindTokenImpl<Binding::ArrayBuffer>::BindTokenImpl;
        friend FriendType;
    };

Which is more verbose, so do this instead.
*/
#define JOSH3D_DEFINE_BIND_TOKEN(BindingName, FriendDecl)                      \
    template<>                                                                 \
    class BindToken<Binding::BindingName> {                                    \
    private:                                                                   \
        GLuint id_;                                                            \
        BindToken(GLuint id) noexcept : id_{ id } {}                           \
        FriendDecl                                                             \
    public:                                                                    \
        GLuint id() const noexcept { return id_; }                             \
        void unbind() const noexcept { unbind_from_context<Binding::BindingName>(); } \
    };


// These are the mixin types that can call `bind()`, `use()`, etc.
// We befriend them and not the final RawObject types, as friendship isn't transitive like that.
namespace detail {
template<typename> struct BufferDSAInterface_Bind;
template<typename> struct VertexArrayDSAInterface_Bind;
template<typename> struct ProgramDSAInterface_Use;
template<typename> struct FramebufferDSAInterface_Bind;
} // namespace detail

// We only define BindTokens for targets that are used somewhere in the interface.
// If a bind-dependant operation can be replaced with a DSA-style one, we do that
// instead and skip the respective BindToken definition.

JOSH3D_DEFINE_BIND_TOKEN(DispatchIndirectBuffer, template<typename> friend struct detail::BufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(DrawIndirectBuffer,     template<typename> friend struct detail::BufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(ParameterBuffer,        template<typename> friend struct detail::BufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(PixelPackBuffer,        template<typename> friend struct detail::BufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(PixelUnpackBuffer,      template<typename> friend struct detail::BufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(VertexArray,            template<typename> friend struct detail::VertexArrayDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(Program,                template<typename> friend struct detail::ProgramDSAInterface_Use;)
JOSH3D_DEFINE_BIND_TOKEN(ReadFramebuffer,        template<typename> friend struct detail::FramebufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(DrawFramebuffer,        template<typename> friend struct detail::FramebufferDSAInterface_Bind;)

#undef JOSH3D_DEFINE_BIND_TOKEN




// TODO: Support indexed binding.
template<Binding B>
class BindGuard {
public:
    BindGuard(BindToken<B> token) : token_{ token } {}

    BindGuard(const BindGuard&)            = delete;
    BindGuard(BindGuard&&)                 = delete;
    BindGuard& operator=(const BindGuard&) = delete;
    BindGuard& operator=(BindGuard&&)      = delete;

    operator BindToken<B>() const noexcept { return token_; }
    BindToken<B> token() const noexcept { return token_; }
    GLuint id() const noexcept { return token_.id(); }

    ~BindGuard() noexcept { token_.unbind(); }

private:
    BindToken<B> token_;
};




} // namespace josh
