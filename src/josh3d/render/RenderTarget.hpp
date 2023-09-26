#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)
#include "EnumUtils.hpp"
#include "GLFramebuffer.hpp"
#include "GLMutability.hpp"
#include "GLRenderbuffer.hpp"
#include "GLTextures.hpp"
#include "GLObjects.hpp"
#include "Attachments.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>




namespace josh {




namespace detail {


inline void attach_generic(
    BoundDrawFramebuffer<GLMutable>& bfbo, RawTexture2D<GLMutable> texture, GLenum attachment)
{
    bfbo.attach_texture(texture, attachment);
}

inline void attach_generic(
    BoundDrawFramebuffer<GLMutable>& bfbo, RawTexture2DMS<GLMutable> texture, GLenum attachment)
{
    bfbo.attach_multisample_texture(texture, attachment);
}

inline void attach_generic(
    BoundDrawFramebuffer<GLMutable>& bfbo, RawTexture2DArray<GLMutable> texture, GLenum attachment)
{
    bfbo.attach_texture_array(texture, attachment);
}

inline void attach_generic(
    BoundDrawFramebuffer<GLMutable>& bfbo, RawCubemap<GLMutable> texture, GLenum attachment)
{
    bfbo.attach_cubemap(texture, attachment);
}

inline void attach_generic(
    BoundDrawFramebuffer<GLMutable>& bfbo, RawCubemapArray<GLMutable> texture, GLenum attachment)
{
    bfbo.attach_cubemap_array(texture, attachment);
}

inline void attach_generic(
    BoundDrawFramebuffer<GLMutable>& bfbo, RawRenderbuffer<GLMutable> renderbuf, GLenum attachment)
{
    bfbo.attach_renderbuffer(renderbuf, attachment);
}



template<typename ...ColorAttachmentTs>
inline void attach_colors(
    BoundDrawFramebuffer<GLMutable>& bfbo, ColorAttachmentTs&... attachments)
{
    GLint index{ 0 };
    (attach_generic(bfbo, attachments.texture(), gl::GL_COLOR_ATTACHMENT0 + index++), ...);
}

template<typename DepthAttachmentT>
inline void attach_depth_or_depth_stencil(
    BoundDrawFramebuffer<GLMutable>& bfbo, DepthAttachmentT& attachment)
{
    GLenum attachment_point = [&](GLenum internal_format) -> GLenum {
        switch(internal_format) {
            using enum GLenum;
            case GL_DEPTH_COMPONENT:
            case GL_DEPTH_COMPONENT16:
            case GL_DEPTH_COMPONENT24:
            case GL_DEPTH_COMPONENT32:
            case GL_DEPTH_COMPONENT32F:
                return GL_DEPTH_ATTACHMENT;
            case GL_DEPTH_STENCIL:
            case GL_DEPTH24_STENCIL8:
            case GL_DEPTH32F_STENCIL8:
                return GL_DEPTH_STENCIL_ATTACHMENT;
            default:
                assert(false &&
                    "Invalid internal_format for Depth or Depth+Stencil attachment.");
        }
    }(attachment.spec().internal_format);

    attach_generic(bfbo, attachment.texture(), attachment_point);
}





template<typename T, size_t Size>
inline auto make_array_filled_with(const T& init_value)
    -> std::array<T, Size>
{
    return std::invoke(
        [&]<size_t ...Idx>(std::index_sequence<Idx...>) {
            return std::array<T, Size>{ ((void)Idx /* ignore */, init_value)... };
        },
        std::make_index_sequence<Size>()
    );
}


} // namespace detail




template<typename DepthAttT>
concept valid_depth_attachment_type =
    std::same_as<DepthAttT, NoDepthAttachment>                       ||
    template_template_specialization_of<DepthAttT, UniqueAttachment> ||
    template_template_specialization_of<DepthAttT, ViewAttachment>;

template<typename ColorAttT>
concept valid_color_attachment_type =
    template_template_specialization_of<ColorAttT, UniqueAttachment> ||
    template_template_specialization_of<ColorAttT, ViewAttachment>;




/*
TODO: Provide a description.
*/
template<
    valid_depth_attachment_type DepthAttachmentT,
    valid_color_attachment_type ...ColorAttachmentTs
>
class RenderTarget {
public:
    static constexpr bool has_depth_attachment{
        !same_as_remove_cvref<DepthAttachmentT, NoDepthAttachment>
    };

    static constexpr size_t num_color_attachments{
        sizeof...(ColorAttachmentTs)
    };

private:
    DepthAttachmentT depth_;

    // Behold, one of the few morally valid uses of std::tuple!
    // Prepare to enjoy the TMP vomit that follows shortly after! Ha-Haaaaaa...
    std::tuple<ColorAttachmentTs...> colors_;
    std::array<bool, num_color_attachments> enabled_for_draw_{
        detail::make_array_filled_with<bool, num_color_attachments>(true)
    };

    Framebuffer fbo_;


    void update_enabled_draw_buffers(BoundDrawFramebuffer<GLMutable>& bfbo) noexcept {
        using enum GLenum;
        std::invoke(
            [&]<size_t ...Idx>(std::index_sequence<Idx...>) {
                bfbo.set_draw_buffers(
                    (enabled_for_draw_[Idx] ?
                        GLenum(GL_COLOR_ATTACHMENT0 + Idx) : GL_NONE)...
                );
            },
            std::make_index_sequence<num_color_attachments>()
        );
    }

    void attach_all_color_buffers(BoundDrawFramebuffer<GLMutable>& bfbo) noexcept {
        std::apply(
            detail::attach_colors<ColorAttachmentTs...>,
            std::tuple_cat(
                std::forward_as_tuple(bfbo),
                std::make_from_tuple<std::tuple<ColorAttachmentTs&...>>(colors_)
            )
        );
    }


public:
    RenderTarget(
        DepthAttachmentT     depth_attachment,
        ColorAttachmentTs... color_attachments)
        : depth_ { std::move(depth_attachment)     }
        , colors_{ std::move(color_attachments)... }
    {
        using enum GLenum;

        fbo_.bind_draw()
            .and_then([this](BoundDrawFramebuffer<GLMutable>& bfbo) {

                // FIXME: No support for pure Stencil attachments. Is that okay?
                if constexpr (has_depth_attachment) {
                    detail::attach_depth_or_depth_stencil(bfbo, depth_);
                }

                attach_all_color_buffers(bfbo);

                update_enabled_draw_buffers(bfbo);
            })
            .unbind();

    }


    void resize_all(const Size2I& new_size) {

        auto resize_impl = [&]<typename AttT>(AttT& attachment) {
            constexpr bool is_size_2 =
                same_as_remove_cvref<typename AttT::size_type, Size2I>;
            constexpr bool is_size_3 =
                same_as_remove_cvref<typename AttT::size_type, Size3I>;

            if constexpr (is_size_2) {
                attachment.resize(Size2I{ new_size });
            } else if constexpr (is_size_3) {
                // Preserve depth.
                attachment.resize(Size3I{ new_size, attachment.size().depth });
            } else {
                assert(false && "Unexpected size_type for a RenderTarget Attachment.");
            }
        };

        if constexpr (has_depth_attachment) {
            resize_impl(depth_);
        }

        std::invoke(
            [&, this]<size_t ...Idx>(std::index_sequence<Idx...>) {
                (resize_impl(std::get<Idx>(colors_)), ...);
            },
            std::make_index_sequence<num_color_attachments>()
        );

    }

    // Odd hack, but whatever. The overload will be disabled anyway.
    using depth_size_type =
        std::conditional_t<has_depth_attachment, typename DepthAttachmentT::size_type, Size2I>;

    void resize_depth_attachment(const depth_size_type& new_size)
        requires has_depth_attachment
    {
        depth_.resize(new_size);
    }

    auto depth_attachment() const noexcept
        -> const DepthAttachmentT&
        requires has_depth_attachment
    {
        return depth_;
    }

    template<auto EnumV>
        requires enumeration<decltype(EnumV)> &&
            (to_underlying(EnumV) < num_color_attachments)
    void resize_color_attachment(
        const std::tuple_element_t<to_underlying(EnumV), std::tuple<ColorAttachmentTs...>>::size_type& new_size)
    {
        std::get<to_underlying(EnumV)>(colors_).resize(new_size);
    }

    template<auto AttachmentId>
        requires std::integral<decltype(AttachmentId)> &&
            (AttachmentId < num_color_attachments)
    void resize_color_attachment(
        const std::tuple_element_t<AttachmentId, std::tuple<ColorAttachmentTs...>>::size_type& new_size)
    {
        std::get<AttachmentId>(colors_).resize(new_size);
    }

    template<auto EnumV>
        requires enumeration<decltype(EnumV)> &&
            (to_underlying(EnumV) < num_color_attachments)
    const auto& color_attachment() const noexcept {
        return std::get<to_underlying(EnumV)>(colors_);
    }

    template<auto AttachmentId>
        requires std::integral<decltype(AttachmentId)> &&
            (AttachmentId < num_color_attachments)
    const auto& color_attachment() const noexcept {
        return std::get<AttachmentId>(colors_);
    }

};




} // namespace josh
