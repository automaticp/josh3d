#pragma once
#include "GLAPI.hpp"
#include "GLScalars.hpp"


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



template<Binding B> inline void unbind_from_context() noexcept;


template<> inline void unbind_from_context<Binding::ArrayBuffer>()            noexcept { gl::glBindBuffer           (gl::GL_ARRAY_BUFFER,                 0); }
template<> inline void unbind_from_context<Binding::AtomicCounterBuffer>()    noexcept { gl::glBindBuffer           (gl::GL_ATOMIC_COUNTER_BUFFER,        0); }
template<> inline void unbind_from_context<Binding::CopyReadBuffer>()         noexcept { gl::glBindBuffer           (gl::GL_COPY_READ_BUFFER,             0); }
template<> inline void unbind_from_context<Binding::CopyWriteBuffer>()        noexcept { gl::glBindBuffer           (gl::GL_COPY_WRITE_BUFFER,            0); }
template<> inline void unbind_from_context<Binding::DispatchIndirectBuffer>() noexcept { gl::glBindBuffer           (gl::GL_DISPATCH_INDIRECT_BUFFER,     0); }
template<> inline void unbind_from_context<Binding::DrawIndirectBuffer>()     noexcept { gl::glBindBuffer           (gl::GL_DRAW_INDIRECT_BUFFER,         0); }
template<> inline void unbind_from_context<Binding::ElementArrayBuffer>()     noexcept { gl::glBindBuffer           (gl::GL_ELEMENT_ARRAY_BUFFER,         0); }
template<> inline void unbind_from_context<Binding::ParameterBuffer>()        noexcept { gl::glBindBuffer           (gl::GL_PARAMETER_BUFFER,             0); }
template<> inline void unbind_from_context<Binding::PixelPackBuffer>()        noexcept { gl::glBindBuffer           (gl::GL_PIXEL_PACK_BUFFER,            0); }
template<> inline void unbind_from_context<Binding::PixelUnpackBuffer>()      noexcept { gl::glBindBuffer           (gl::GL_PIXEL_UNPACK_BUFFER,          0); }
template<> inline void unbind_from_context<Binding::QueryBuffer>()            noexcept { gl::glBindBuffer           (gl::GL_QUERY_BUFFER,                 0); }
template<> inline void unbind_from_context<Binding::ShaderStorageBuffer>()    noexcept { gl::glBindBuffer           (gl::GL_SHADER_STORAGE_BUFFER,        0); }
template<> inline void unbind_from_context<Binding::TextureBuffer>()          noexcept { gl::glBindBuffer           (gl::GL_TEXTURE_BUFFER,               0); }
template<> inline void unbind_from_context<Binding::UniformBuffer>()          noexcept { gl::glBindBuffer           (gl::GL_UNIFORM_BUFFER,               0); }
template<> inline void unbind_from_context<Binding::VertexArray>()            noexcept { gl::glBindVertexArray      (                                     0); }
template<> inline void unbind_from_context<Binding::DrawFramebuffer>()        noexcept { gl::glBindFramebuffer      (gl::GL_DRAW_FRAMEBUFFER,             0); }
template<> inline void unbind_from_context<Binding::ReadFramebuffer>()        noexcept { gl::glBindFramebuffer      (gl::GL_READ_FRAMEBUFFER,             0); }
template<> inline void unbind_from_context<Binding::TransformFeedback>()      noexcept { gl::glBindTransformFeedback(gl::GL_TRANSFORM_FEEDBACK,           0); }
template<> inline void unbind_from_context<Binding::Renderbuffer>()           noexcept { gl::glBindRenderbuffer     (gl::GL_RENDERBUFFER,                 0); }
template<> inline void unbind_from_context<Binding::BufferTexture>()          noexcept { gl::glBindTexture          (gl::GL_TEXTURE_BUFFER,               0); }
template<> inline void unbind_from_context<Binding::Texture1D>()              noexcept { gl::glBindTexture          (gl::GL_TEXTURE_1D,                   0); }
template<> inline void unbind_from_context<Binding::Texture1DArray>()         noexcept { gl::glBindTexture          (gl::GL_TEXTURE_1D_ARRAY,             0); }
template<> inline void unbind_from_context<Binding::TextureRectangle>()       noexcept { gl::glBindTexture          (gl::GL_TEXTURE_RECTANGLE,            0); }
template<> inline void unbind_from_context<Binding::Texture2D>()              noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D,                   0); }
template<> inline void unbind_from_context<Binding::Texture2DArray>()         noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D_ARRAY,             0); }
template<> inline void unbind_from_context<Binding::Texture2DMS>()            noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D_MULTISAMPLE,       0); }
template<> inline void unbind_from_context<Binding::Texture2DMSArray>()       noexcept { gl::glBindTexture          (gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0); }
template<> inline void unbind_from_context<Binding::Texture3D>()              noexcept { gl::glBindTexture          (gl::GL_TEXTURE_3D,                   0); }
template<> inline void unbind_from_context<Binding::Cubemap>()                noexcept { gl::glBindTexture          (gl::GL_TEXTURE_CUBE_MAP,             0); }
template<> inline void unbind_from_context<Binding::CubemapArray>()           noexcept { gl::glBindTexture          (gl::GL_TEXTURE_CUBE_MAP_ARRAY,       0); }
template<> inline void unbind_from_context<Binding::Program>()                noexcept { gl::glUseProgram           (                                     0); }
template<> inline void unbind_from_context<Binding::ProgramPipeline>()        noexcept { gl::glBindProgramPipeline  (                                     0); }
// template<> inline void unbind_from_context<Binding::Sampler>()                noexcept {                                                                     }



// TODO: Indexed Unbind.






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
namespace dsa::detail {
template<typename> struct BufferDSAInterface_Bind;
template<typename> struct VertexArrayDSAInterface_Bind;
template<typename> struct ShaderProgramDSAInterface_Use;
template<typename> struct FramebufferDSAInterface_Bind;
} // namespace dsa::detail

// We only define BindTokens for targets that are used somewhere in the interface.
// If a bind-dependant operation can be replaced with a DSA-style one, we do that
// instead and skip the respective BindToken definition.

JOSH3D_DEFINE_BIND_TOKEN(DispatchIndirectBuffer, template<typename> friend struct dsa::detail::BufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(DrawIndirectBuffer,     template<typename> friend struct dsa::detail::BufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(ParameterBuffer,        template<typename> friend struct dsa::detail::BufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(PixelPackBuffer,        template<typename> friend struct dsa::detail::BufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(PixelUnpackBuffer,      template<typename> friend struct dsa::detail::BufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(VertexArray,            template<typename> friend struct dsa::detail::VertexArrayDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(Program,                template<typename> friend struct dsa::detail::ShaderProgramDSAInterface_Use;)
JOSH3D_DEFINE_BIND_TOKEN(ReadFramebuffer,        template<typename> friend struct dsa::detail::FramebufferDSAInterface_Bind;)
JOSH3D_DEFINE_BIND_TOKEN(DrawFramebuffer,        template<typename> friend struct dsa::detail::FramebufferDSAInterface_Bind;)


} // namespace josh
