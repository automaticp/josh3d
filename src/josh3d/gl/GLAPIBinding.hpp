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

    Sampler                = GLuint(gl::GL_SAMPLER_BINDING),

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

    // GL_VERTEX_BINDING_BUFFER, // Who the fuck are you???
};




template<Binding B>
class BindToken {
private:
    GLuint id_;
    BindToken(GLuint id) noexcept : id_{ id } {}
public:
    GLuint id() const noexcept { return id_; }
};




template<Binding B>
inline void unbind() noexcept;





} // namespace josh
