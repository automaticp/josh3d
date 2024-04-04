#pragma once


namespace josh {


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
    Program,
    FenceSync,
    Query,
    Sampler
};


template<typename T, GLKind ...KindV>
concept of_kind = ((T::kind_type == KindV) || ...);




} // namespace josh
