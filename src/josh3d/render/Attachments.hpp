#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)
#include "GLMutability.hpp"
#include "GLTextures.hpp"
#include "GLRenderbuffer.hpp"
#include "GLObjects.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <concepts>


namespace josh {


namespace detail {

inline void allocate_attachment_image(
    RawTexture2D<GLMutable>& tex,
    const RawTexture2D<GLMutable>::size_type& size,
    const RawTexture2D<GLMutable>::spec_type& spec)
{
    tex.bind().allocate_image(size, spec);
}

inline void allocate_attachment_image(
    RawTexture2DMS<GLMutable>& tex,
    const RawTexture2DMS<GLMutable>::size_type& size,
    const RawTexture2DMS<GLMutable>::spec_type& spec)
{
    tex.bind().allocate_image(size, spec);
}

inline void allocate_attachment_image(
    RawTexture2DArray<GLMutable>& tex,
    const RawTexture2DArray<GLMutable>::size_type& size,
    const RawTexture2DArray<GLMutable>::spec_type& spec)
{
    tex.bind().allocate_all_images(size, spec);
}

inline void allocate_attachment_image(
    RawCubemap<GLMutable>& tex,
    const RawCubemap<GLMutable>::size_type& size,
    const RawCubemap<GLMutable>::spec_type& spec)
{
    tex.bind().allocate_all_images(size, spec);
}

inline void allocate_attachment_image(
    RawCubemapArray<GLMutable>& tex,
    const RawCubemapArray<GLMutable>::size_type& size,
    const RawCubemapArray<GLMutable>::spec_type& spec)
{
    tex.bind().allocate_all_images(size, spec);
}

inline void allocate_attachment_image(
    RawRenderbuffer<GLMutable>& rbo,
    const RawRenderbuffer<GLMutable>::size_type& size,
    const RawRenderbuffer<GLMutable>::spec_type& spec)
{
    assert(spec.num_samples > 0);
    rbo.bind().and_then([&](BoundRenderbuffer<GLMutable>& brbo) {
        if (spec.num_samples == 1) {
            brbo.allocate_storage(size, spec);
        } else {
            brbo.allocate_multisample_storage(size, spec);
        }
    });
}



} // namespace detail


/*
Attachment type that carries unique ownership semantics and
controls the lifetime of the underlying texture/renderbuffer handle.
*/
template<template<typename> typename RawTextureTmp>
    requires detail::gl_texture_object<RawTextureTmp<GLMutable>>
class UniqueAttachment {
private:
    using TextureT = RawTextureTmp<GLMutable>;
    GLUnique<TextureT> texture_;
    TextureT::size_type size_;
    TextureT::spec_type spec_;

public:
    using size_type = TextureT::size_type;
    using spec_type = TextureT::spec_type;

    UniqueAttachment(const size_type& size, const spec_type& spec)
        : size_{ size }, spec_{ spec }
    {
        reallocate_storage();
    }

    RawTextureTmp<GLMutable> texture()       noexcept { return texture_; }
    RawTextureTmp<GLConst>   texture() const noexcept { return texture_; }
    const spec_type& spec() const noexcept { return spec_; }
    const size_type& size() const noexcept { return size_; }

    void resize(const size_type& new_size) {
        if (size_ != new_size) {
            size_ = new_size;
            reallocate_storage();
        }
    }

    void respec(const spec_type& new_spec) {
        if (spec_ != new_spec) {
            spec_ = new_spec;
            reallocate_storage();
        }
    }

    void resize_and_respec(
        const size_type& new_size, const spec_type& new_spec)
    {
        if (size_ != new_size || spec_ != new_spec) {
            size_ = new_size;
            spec_ = new_spec;
            reallocate_storage();
        }
    }

private:
    void reallocate_storage() {
        detail::allocate_attachment_image(texture_, size_, spec_);
    }
};


/*
Attachment type that carries no ownership and simply
observes an external texture/renderbuffer.
*/
template<template<typename> typename RawTextureTmp>
    requires detail::gl_texture_object<RawTextureTmp<GLMutable>>
class ViewAttachment {
private:
    using TextureT = RawTextureTmp<GLMutable>;
    TextureT texture_;
    TextureT::size_type size_;
    TextureT::spec_type spec_;

public:
    using size_type = TextureT::size_type;
    using spec_type = TextureT::spec_type;

    ViewAttachment(
        RawTextureTmp<GLMutable> texture,
        const size_type& size,
        const spec_type& spec)
        : texture_{ std::move(texture) }
        , size_{ size }
        , spec_{ spec }
    {
        reallocate_storage();
    }

    // This constructor assumes that the `texture`
    // already has the storage allocated and will
    // query the `size` and `spec` from it.
    ViewAttachment(RawTextureTmp<GLMutable> texture)
        : texture_{ std::move(texture) }
        , size_{ texture_.bind().get_size() }
        , spec_{ texture_.bind().get_spec() }
    {}

    ViewAttachment(UniqueAttachment<RawTextureTmp>& uattach)
        : texture_{ uattach.texture() }
        , size_{ uattach.size() }
        , spec_{ uattach.spec() }
    {}

    RawTextureTmp<GLMutable> texture()       noexcept { return texture_; }
    RawTextureTmp<GLConst>   texture() const noexcept { return texture_; }
    const spec_type& spec() const noexcept { return spec_; }
    const size_type& size() const noexcept { return size_; }

    void resize(const size_type& new_size) {
        if (size_ != new_size) {
            size_ = new_size;
            reallocate_storage();
        }
    }

    void respec(const spec_type& new_spec) {
        if (spec_ != new_spec) {
            spec_ = new_spec;
            reallocate_storage();
        }
    }

    void resize_and_respec(
        const size_type& new_size, const spec_type& new_spec)
    {
        if (size_ != new_size || spec_ != new_spec) {
            size_ = new_size;
            spec_ = new_spec;
            reallocate_storage();
        }
    }

private:
    void reallocate_storage() {
        detail::allocate_attachment_image(texture_, size_, spec_);
    }
};



/*
Special attachment type used to create a RenderTarget
with no depth or depth/stencil attachment.
*/
struct NoDepthAttachment {
    using size_type = void;
    using spec_type = void;
};





} // namespace josh

