#pragma once
#include "EnumUtils.hpp"


namespace josh {

/*
Object "kinds" identify a vague "category" of objects
and specify allocation (creation/deletion) functions.
*/
enum class GLKind
{
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
JOSH3D_DEFINE_ENUM_EXTRAS(GLKind,
    Texture,
    Buffer,
    VertexArray,
    Framebuffer,
    DefaultFramebuffer,
    Renderbuffer,
    Shader,
    Program,
    FenceSync,
    Query,
    Sampler);

template<typename T, GLKind ...KindV>
concept of_kind = ((T::kind_type == KindV) || ...);


} // namespace josh
