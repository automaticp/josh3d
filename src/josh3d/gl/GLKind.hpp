#pragma once


namespace josh::dsa {


/*
Object "kinds" that define basic operations, creation
and deletion of objects.

Names are Uppercase to match Type capitalization. Helps in macros.
*/
enum class GLKind {
    Texture,
    Buffer,
    VertexArray,
    Framebuffer,
    DefaultFramebuffer, // No allocation.
    Renderbuffer,
    Shader,
    ShaderProgram,
    FenceSync,
    Query,
    Sampler
};


template<typename T, GLKind KindV>
concept of_kind = requires {
    requires T::kind_type == KindV;
};




} // namespace josh::dsa
