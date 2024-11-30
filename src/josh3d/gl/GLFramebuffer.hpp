#pragma once
#include "DecayToRaw.hpp"
#include "GLAPI.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPILimits.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLKind.hpp"
#include "GLScalars.hpp"
#include "GLMutability.hpp"
#include "EnumUtils.hpp"
#include "Region.hpp"
#include "detail/MagicConstructorsMacro.hpp"
#include "detail/RawGLHandle.hpp"
#include "GLTextures.hpp"
#include <glbinding/gl/bitfield.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <concepts>
#include <span>




namespace josh {


enum class BlitFilter : GLuint {
    Nearest = GLuint(gl::GL_NEAREST),
    Linear  = GLuint(gl::GL_LINEAR),
};


enum class FramebufferStatus : GLuint {
    Complete                    = GLuint(gl::GL_FRAMEBUFFER_COMPLETE),
    Undefined                   = GLuint(gl::GL_FRAMEBUFFER_UNDEFINED),
    Unsupported                 = GLuint(gl::GL_FRAMEBUFFER_UNSUPPORTED),
    IncompleteAttachment        = GLuint(gl::GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT),
    IncompleteMissingAttachment = GLuint(gl::GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT),
    IncompleteDrawBuffer        = GLuint(gl::GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER),
    IncompleteReadBuffer        = GLuint(gl::GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER),
    IncompleteMultisample       = GLuint(gl::GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE),
    IncompleteLayerTargets      = GLuint(gl::GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS),
};


/*
Monoscopic contexts include only left buffers, and stereoscopic contexts include both
left and right buffers. Likewise, single-buffered contexts include only front buffers, and
double-buffered contexts include both front and back buffers.
The context is selected at GL initialization.
*/
enum class DefaultFramebufferBufferSet : GLuint {
    FrontLeft    = GLuint(gl::GL_FRONT_LEFT),
    FrontRight   = GLuint(gl::GL_FRONT_RIGHT),
    BackLeft     = GLuint(gl::GL_BACK_LEFT),
    BackRight    = GLuint(gl::GL_BACK_RIGHT),
    Front        = GLuint(gl::GL_FRONT),
    Back         = GLuint(gl::GL_BACK),
    Left         = GLuint(gl::GL_LEFT),
    Right        = GLuint(gl::GL_RIGHT),
    FrontAndBack = GLuint(gl::GL_FRONT_AND_BACK),
};


enum class DefaultFramebufferBuffer : GLuint {
    FrontLeft  = GLuint(gl::GL_FRONT_LEFT),
    FrontRight = GLuint(gl::GL_FRONT_RIGHT),
    BackLeft   = GLuint(gl::GL_BACK_LEFT),
    BackRight  = GLuint(gl::GL_BACK_RIGHT),
};








template<mutability_tag MutT>
class RawFramebuffer;

template<mutability_tag MutT>
class RawDefaultFramebuffer;


namespace detail {


template<typename CRTP>
struct FramebufferDSAInterface_Bind {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:

    // Wraps `glBindFramebuffer` with `target = GL_READ_FRAMEBUFFER`.
    [[nodiscard("BindTokens have to be provided to an API call that expects bound state.")]]
    auto bind_read() const noexcept
        -> BindToken<Binding::ReadFramebuffer>
    {
        gl::glBindFramebuffer(gl::GL_READ_FRAMEBUFFER, self_id());
        return { self_id() };
    }

    // Wraps `glBindFramebuffer` with `target = GL_DRAW_FRAMEBUFFER`.
    [[nodiscard("BindTokens have to be provided to an API call that expects bound state.")]]
    auto bind_draw() const noexcept
        -> BindToken<Binding::DrawFramebuffer>
            requires mt::is_mutable
    {
        gl::glBindFramebuffer(gl::GL_DRAW_FRAMEBUFFER, self_id());
        return { self_id() };
    }

};




template<typename CRTP>
struct FramebufferDSAInterface_Common {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:

    // Wraps `glBlitNamedFramebuffer`.
    void blit_to(
        RawFramebuffer<GLMutable> dst,
        const Region2I&           src_region,
        const Region2I&           dst_region,
        BufferMask                buffers,
        BlitFilter                filter) const noexcept;

    // Wraps `glBlitNamedFramebuffer`.
    void blit_to(
        RawDefaultFramebuffer<GLMutable> dst,
        const Region2I&                  src_region,
        const Region2I&                  dst_region,
        BufferMask                       buffers,
        BlitFilter                       filter) const noexcept;


    // Wraps `glCheckNamedFramebufferStatus` with `target = GL_DRAW_FRAMEBUFFER`.
    FramebufferStatus get_status_for_draw() const noexcept {
        return enum_cast<FramebufferStatus>(
            gl::glCheckNamedFramebufferStatus(self_id(), gl::GL_DRAW_FRAMEBUFFER)
        );
    }

    // Wraps `glCheckNamedFramebufferStatus` with `target = GL_READ_FRAMEBUFFER`
    FramebufferStatus get_status_for_read() const noexcept {
        return enum_cast<FramebufferStatus>(
            gl::glCheckNamedFramebufferStatus(self_id(), gl::GL_READ_FRAMEBUFFER)
        );
    }

    // Wraps `glCheckNamedFramebufferStatus` with `target = GL_DRAW_FRAMEBUFFER`.
    bool is_complete_for_draw() const noexcept {
        return get_status_for_draw() == FramebufferStatus::Complete;
    }

    // Wraps `glCheckNamedFramebufferStatus` with `target = GL_READ_FRAMEBUFFER`
    bool is_complete_for_read() const noexcept {
        return get_status_for_read() == FramebufferStatus::Complete;
    }

};




template<typename CRTP>
struct FramebufferDSAInterface_Attachments {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:

    // Wraps `glNamedFramebufferDrawBuffer` with `buf = GL_COLOR_ATTACHMENT0 + attachment_index`.
    void specify_single_color_buffer_for_draw(GLuint attachment_index) const noexcept
        requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferDrawBuffer(
            self_id(), GLenum{ GLuint(gl::GL_COLOR_ATTACHMENT0) + attachment_index }
        );
    }

    // Wraps `glNamedFramebufferDrawBuffers` with `bufs[i] = GL_COLOR_ATTACHMENT0 + attachment_indices[i]`.
    template<std::convertible_to<GLuint> ...UIntT>
    void specify_color_buffers_for_draw(UIntT... attachment_indices) const noexcept
        requires mt::is_mutable
    {
        assert(self_id() != 0);
        constexpr auto bufs_size = sizeof...(UIntT);
        const GLenum bufs[bufs_size]{ (gl::GL_COLOR_ATTACHMENT0 + attachment_indices)... };
        gl::glNamedFramebufferDrawBuffers(self_id(), bufs_size, bufs);
    }

    // Wraps `glNamedFramebufferDrawBuffers` with `bufs = attachment_constants.data()`.
    // Overload for runtime-sized arrays. You have to pick the right GLenums yourself.
    void specify_color_buffers_for_draw(std::span<const GLenum> attachment_constants) const noexcept
        requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferDrawBuffers(
            self_id(), attachment_constants.size(), attachment_constants.data()
        );
    }

    // Wraps `glNamedFramebufferDrawBuffer` with `buf = GL_NONE`.
    void disable_all_color_buffers_for_draw() const noexcept
        requires mt::is_mutable
    {
        assert(self_id() == 0);
        gl::glNamedFramebufferDrawBuffer(self_id(), gl::GL_NONE);
    }




    // Wraps `glNamedFramebufferReadBuffer` with `src = GL_COLOR_ATTACHMENT0 + attachment_index`.
    void specify_color_buffer_for_read(GLuint attachment_index) const noexcept {
        assert(self_id() != 0);
        assert(attachment_index < glapi::limits::max_color_attachments());
        gl::glNamedFramebufferReadBuffer(
            self_id(), GLenum{ GLuint(gl::GL_COLOR_ATTACHMENT0) + attachment_index }
        );
    }

    // Wraps `glNamedFramebufferReadBuffer` with `src = GL_NONE`.
    void disable_all_color_buffers_for_read() const noexcept {
        assert(self_id() == 0);
        gl::glNamedFramebufferReadBuffer(self_id(), gl::GL_NONE);
    }




    // Wraps `glNamedFramebufferTexture` with `attachment = GL_COLOR_ATTACHMENT0 + attachment_index`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            texture_traits<TextureT>::has_lod
    void attach_texture_to_color_buffer(
        const TextureT& texture,
        GLuint          attachment_index,
        MipLevel        mip_level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        assert(attachment_index < glapi::limits::max_color_attachments());
        gl::glNamedFramebufferTexture(
            self_id(),
            GLenum{ GLuint(gl::GL_COLOR_ATTACHMENT0) + attachment_index },
            decay_to_raw(texture).id(),
            mip_level
        );
    }

    // Wraps `glNamedFramebufferTexture` with `attachment = GL_COLOR_ATTACHMENT0 + attachment_index`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            (!texture_traits<TextureT>::has_lod)
    void attach_texture_to_color_buffer(
        const TextureT& texture,
        GLuint          attachment_index) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        assert(attachment_index < glapi::limits::max_color_attachments());
        gl::glNamedFramebufferTexture(
            self_id(),
            GLenum{ GLuint(gl::GL_COLOR_ATTACHMENT0) + attachment_index },
            decay_to_raw(texture).id(),
            0
        );
    }




    // Wraps `glNamedFramebufferTexture` with `attachment = GL_DEPTH_ATTACHMENT`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            texture_traits<TextureT>::has_lod
    void attach_texture_to_depth_buffer(
        const TextureT& texture,
        MipLevel        mip_level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferTexture(
            self_id(),
            gl::GL_DEPTH_ATTACHMENT,
            decay_to_raw(texture).id(),
            mip_level
        );
    }

    // Wraps `glNamedFramebufferTexture` with `attachment = GL_DEPTH_ATTACHMENT`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            (!texture_traits<TextureT>::has_lod)
    void attach_texture_to_depth_buffer(
        const TextureT& texture) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferTexture(
            self_id(),
            gl::GL_DEPTH_ATTACHMENT,
            decay_to_raw(texture).id(),
            0
        );
    }




    // Wraps `glNamedFramebufferTexture` with `attachment = GL_STENCIL_ATTACHMENT`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            texture_traits<TextureT>::has_lod
    void attach_texture_to_stencil_buffer(
        const TextureT& texture,
        MipLevel        mip_level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferTexture(
            self_id(),
            gl::GL_STENCIL_ATTACHMENT,
            decay_to_raw(texture).id(),
            mip_level
        );
    }

    // Wraps `glNamedFramebufferTexture` with `attachment = GL_STENCIL_ATTACHMENT`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            (!texture_traits<TextureT>::has_lod)
    void attach_texture_to_stencil_buffer(
        const TextureT& texture) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferTexture(
            self_id(),
            gl::GL_STENCIL_ATTACHMENT,
            decay_to_raw(texture).id(),
            0
        );
    }




    // Wraps `glNamedFramebufferTextureLayer` with `attachment = GL_COLOR_ATTACHMENT0 + attachment_index`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            texture_traits<TextureT>::has_lod &&
            texture_traits<TextureT>::is_layered
    void attach_texture_layer_to_color_buffer(
        const TextureT& texture,
        Layer           layer,
        GLuint          attachment_index,
        MipLevel        mip_level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        assert(attachment_index < glapi::limits::max_color_attachments());
        gl::glNamedFramebufferTextureLayer(
            self_id(),
            GLenum{ GLuint(gl::GL_COLOR_ATTACHMENT0) + attachment_index },
            decay_to_raw(texture).id(),
            mip_level,
            layer
        );
    }


    // Wraps `glNamedFramebufferTextureLayer` with `attachment = GL_COLOR_ATTACHMENT0 + attachment_index`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            (!texture_traits<TextureT>::has_lod) &&
            texture_traits<TextureT>::is_layered
    void attach_texture_layer_to_color_buffer(
        const TextureT& texture,
        Layer           layer,
        GLuint          attachment_index) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        assert(attachment_index < glapi::limits::max_color_attachments());
        gl::glNamedFramebufferTextureLayer(
            self_id(),
            GLenum{ GLuint(gl::GL_COLOR_ATTACHMENT0) + attachment_index },
            decay_to_raw(texture).id(),
            0,
            layer
        );
    }




    // Wraps `glNamedFramebufferTextureLayer` with `attachment = GL_DEPTH_ATTACHMENT`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            texture_traits<TextureT>::has_lod &&
            texture_traits<TextureT>::is_layered
    void attach_texture_layer_to_depth_buffer(
        const TextureT& texture,
        Layer           layer,
        MipLevel        mip_level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferTextureLayer(
            self_id(),
            gl::GL_DEPTH_ATTACHMENT,
            decay_to_raw(texture).id(),
            mip_level,
            layer
        );
    }

    // Wraps `glNamedFramebufferTextureLayer` with `attachment = GL_DEPTH_ATTACHMENT`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            (!texture_traits<TextureT>::has_lod) &&
            texture_traits<TextureT>::is_layered
    void attach_texture_layer_to_depth_buffer(
        const TextureT& texture,
        Layer           layer) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferTextureLayer(
            self_id(),
            gl::GL_DEPTH_ATTACHMENT,
            decay_to_raw(texture).id(),
            0,
            layer
        );
    }




    // Wraps `glNamedFramebufferTextureLayer` with `attachment = GL_STENCIL_ATTACHMENT`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            texture_traits<TextureT>::has_lod &&
            texture_traits<TextureT>::is_layered
    void attach_texture_layer_to_stencil_buffer(
        const TextureT& texture,
        Layer           layer,
        MipLevel        mip_level = MipLevel{ 0 }) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferTextureLayer(
            self_id(),
            gl::GL_STENCIL_ATTACHMENT,
            decay_to_raw(texture).id(),
            mip_level,
            layer
        );
    }

    // Wraps `glNamedFramebufferTextureLayer` with `attachment = GL_STENCIL_ATTACHMENT`.
    template<of_kind<GLKind::Texture> TextureT>
        requires
            mutability_traits<TextureT>::is_mutable &&
            (!texture_traits<TextureT>::has_lod) &&
            texture_traits<TextureT>::is_layered
    void attach_texture_layer_to_stencil_buffer(
        const TextureT& texture,
        Layer           layer) const noexcept
            requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferTextureLayer(
            self_id(),
            gl::GL_STENCIL_ATTACHMENT,
            decay_to_raw(texture).id(),
            0,
            layer
        );
    }


    // TODO:
    void _attach_renderbuffer_to_color_buffer() const noexcept {}
    void _attach_renderbuffer_to_depth_buffer() const noexcept {}
    void _attach_renderbuffer_to_stencil_buffer() const noexcept {}




    // Wraps `glNamedFramebufferTexture` with `attachment = GL_COLOR_ATTACHMENT0 + attachment_index` and `texture = 0`.
    void detach_color_buffer(GLuint attachment_index) const noexcept
        requires mt::is_mutable
    {
        gl::glNamedFramebufferTexture(
            self_id(), GLenum{ GLuint(gl::GL_COLOR_ATTACHMENT0) + attachment_index }, 0, 0
        );
    }

    // Wraps `glNamedFramebufferTexture` with `attachment = GL_DEPTH_ATTACHMENT` and `texture = 0`.
    void detach_depth_buffer() const noexcept
        requires mt::is_mutable
    {
        gl::glNamedFramebufferTexture(
            self_id(), gl::GL_DEPTH_ATTACHMENT, 0, 0
        );
    }

    // Wraps `glNamedFramebufferTexture` with `attachment = GL_DEPTH_ATTACHMENT` and `texture = 0`.
    void detach_stencil_buffer() const noexcept
        requires mt::is_mutable
    {
        gl::glNamedFramebufferTexture(
            self_id(), gl::GL_STENCIL_ATTACHMENT, 0, 0
        );
    }

    // Also: get_attachemnt_type, etc.

};




template<typename CRTP>
struct DefaultFramebufferDSAInterface_Attachments {
private:
    GLuint self_id() const noexcept { return static_cast<const CRTP&>(*this).id(); }
    using mt = mutability_traits<CRTP>;
public:

    // Wraps `glNamedFramebufferDrawBuffer` with `buf = attachment_set`.
    void specify_default_buffer_set_for_draw(DefaultFramebufferBufferSet attachment_set) const noexcept
        requires mt::is_mutable
    {
        assert(self_id() == 0);
        gl::glNamedFramebufferDrawBuffer(self_id(), enum_cast<GLenum>(attachment_set));
    }

    // Wraps `glNamedFramebufferDrawBuffers` with `bufs[i] = attachment_buffers[i]`.
    template<std::same_as<DefaultFramebufferBuffer> ...DefaultFramebufferBufferT>
    void specify_default_buffers_for_draw(DefaultFramebufferBufferT... attachment_buffers) const noexcept
        requires mt::is_mutable
    {
        assert(self_id() == 0);
        constexpr auto bufs_size = sizeof...(DefaultFramebufferBufferT);
        const GLenum bufs[bufs_size]{ enum_cast<GLenum>(attachment_buffers)... };
        gl::glNamedFramebufferDrawBuffers(self_id(), bufs_size, bufs);
    }

    // Wraps `glNamedFramebufferDrawBuffers` with `bufs = attachment_constants.data()`.
    // Overload for runtime-sized arrays. You have to pick the right GLenums yourself.
    void specify_default_buffers_for_draw(std::span<const GLenum> attachment_constants) const noexcept
        requires mt::is_mutable
    {
        assert(self_id() != 0);
        gl::glNamedFramebufferDrawBuffers(
            self_id(), attachment_constants.size(), attachment_constants.data()
        );
    }

    // Wraps `glNamedFramebufferDrawBuffer` with `buf = GL_NONE`.
    void disable_all_default_buffers_for_draw() const noexcept
        requires mt::is_mutable
    {
        assert(self_id() == 0);
        gl::glNamedFramebufferDrawBuffer(self_id(), gl::GL_NONE);
    }



    // Wraps `glNamedFramebufferReadBuffer` with `src = attachment_buffer`.
    void specify_default_buffer_for_read(DefaultFramebufferBuffer attachment_buffer) const noexcept
        requires mt::is_mutable
    {
        assert(self_id() == 0);
        gl::glNamedFramebufferReadBuffer(self_id(), enum_cast<GLenum>(attachment_buffer));
    }

    // Wraps `glNamedFramebufferReadBuffer` with `src = GL_NONE`.
    void disable_all_default_buffers_for_read() const noexcept
        requires mt::is_mutable
    {
        assert(self_id() == 0);
        gl::glNamedFramebufferReadBuffer(self_id(), gl::GL_NONE);
    }


};




template<typename CRTP>
struct FramebufferDSAInterface
    : FramebufferDSAInterface_Common<CRTP>
    , FramebufferDSAInterface_Bind<CRTP>
    , FramebufferDSAInterface_Attachments<CRTP>
{};


template<typename CRTP>
struct DefaultFramebufferDSAInterface
    : FramebufferDSAInterface_Common<CRTP>
    , FramebufferDSAInterface_Bind<CRTP>
    , DefaultFramebufferDSAInterface_Attachments<CRTP>
{};



} // namespace detail









template<mutability_tag MutT = GLMutable>
class RawFramebuffer
    : public detail::RawGLHandle<MutT>
    , public detail::FramebufferDSAInterface<RawFramebuffer<MutT>>
{
public:
    static constexpr GLKind kind_type = GLKind::Framebuffer;
    JOSH3D_MAGIC_CONSTRUCTORS_2(RawFramebuffer, mutability_traits<RawFramebuffer>, detail::RawGLHandle<MutT>)

};




template<mutability_tag MutT = GLMutable>
class RawDefaultFramebuffer
    : public detail::RawGLHandle<MutT>
    , public detail::DefaultFramebufferDSAInterface<RawDefaultFramebuffer<MutT>>
{
public:
    static constexpr GLKind kind_type = GLKind::DefaultFramebuffer;
    JOSH3D_MAGIC_CONSTRUCTORS_CONVERSION(RawDefaultFramebuffer, mutability_traits<RawDefaultFramebuffer>, detail::RawGLHandle<MutT>)
    // Can only be 0.
    RawDefaultFramebuffer() noexcept : detail::RawGLHandle<MutT>{ 0 } {}
};




template<typename CRTP>
inline void detail::FramebufferDSAInterface_Common<CRTP>::blit_to(
    RawFramebuffer<GLMutable> dst,
    const Region2I&           src_region,
    const Region2I&           dst_region,
    BufferMask                buffers,
    BlitFilter                filter) const noexcept
{
    const Offset2I& src_offset     = src_region.offset;
    const Offset2I& dst_offset     = dst_region.offset;
    const Offset2I  src_offset_end = src_region.offset + src_region.extent;
    const Offset2I  dst_offset_end = dst_region.offset + dst_region.extent;
    gl::glBlitNamedFramebuffer(
        self_id(), dst.id(),
        src_offset.x,     src_offset.y,
        src_offset_end.x, src_offset_end.y,
        dst_offset.x,     dst_offset.y,
        dst_offset_end.x, dst_offset_end.y,
        enum_cast<gl::ClearBufferMask>(buffers),
        enum_cast<GLenum>(filter)
    );
}


template<typename CRTP>
inline void detail::FramebufferDSAInterface_Common<CRTP>::blit_to(
    RawDefaultFramebuffer<GLMutable> dst [[maybe_unused]],
    const Region2I&                  src_region,
    const Region2I&                  dst_region,
    BufferMask                       buffers,
    BlitFilter                       filter) const noexcept
{
    blit_to(
        RawFramebuffer<GLMutable>::from_id(0),
        src_region, dst_region,
        buffers, filter
    );
}




} // namespace josh
