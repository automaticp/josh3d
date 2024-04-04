#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)
#include "EnumUtils.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLFramebuffer.hpp"
#include "GLMutability.hpp"
// #include "GLRenderbuffer.hpp"
#include "GLTextures.hpp"
#include "GLObjects.hpp"
#include "Attachments.hpp"
#include "Size.hpp"
#include <array>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <concepts>
#include <glbinding/gl/types.h>
#include <tuple>
#include <type_traits>
#include <utility>




namespace josh {


namespace detail {

template<typename T, size_t Size>
inline auto make_array_filled_with(const T& init_value)
    -> std::array<T, Size>
{
    return
        [&]<size_t ...Idx>(std::index_sequence<Idx...>) {
            return std::array<T, Size>{ ((void)Idx /* ignore */, init_value)... };
        }(std::make_index_sequence<Size>());
}

} // namespace detail




/*
template<typename DepthAttT>
concept valid_depth_attachment_type =
    std::same_as<DepthAttT, NoDepthAttachment>                       ||
    template_template_specialization_of<DepthAttT, UniqueAttachment> ||
    template_template_specialization_of<DepthAttT, ViewAttachment>;

template<typename ColorAttT>
concept valid_color_attachment_type =
    template_template_specialization_of<ColorAttT, UniqueAttachment> ||
    template_template_specialization_of<ColorAttT, ViewAttachment>;
*/


template<auto Idx, size_t NumAttachments>
concept color_attachment_index_in_bounds = requires {
    requires to_underlying_or_value(Idx) >= 0;
    requires to_underlying_or_value(Idx) < NumAttachments;
};




template<typename DepthAttachmentT, typename ...ColorAttachmentTs>
class RenderTarget {
    static constexpr bool is_any_attachment_multisample() noexcept;
    static constexpr bool is_any_attachment_array()       noexcept;
    using color_attachments_type = std::tuple<ColorAttachmentTs...>;
public:
    static constexpr bool   has_depth_attachment  = !same_as_remove_cvref<DepthAttachmentT, NoDepthAttachment>;
    static constexpr size_t num_color_attachments = sizeof...(ColorAttachmentTs);
    static constexpr bool   is_multisample        = is_any_attachment_multisample();
    static constexpr bool   is_array              = is_any_attachment_array();
    // TODO: resolution_ndims and resolution_type?
    // TODO: Support for empty RenderTargets?
    static_assert(has_depth_attachment + num_color_attachments != 0, "RenderTarget with no attachments is not supported.");

    template<auto Idx>
    using color_attachment_type = std::tuple_element_t<to_underlying_or_value(Idx), color_attachments_type>;
    using depth_attachment_type = DepthAttachmentT;

    template<auto Idx>
    using color_attachment_traits = color_attachment_type<Idx>::texture_traits;
    using depth_attachment_traits = depth_attachment_type::texture_traits;

    template<auto Idx>
    using color_attachment_kind_traits = color_attachment_type<Idx>::kind_traits;
    using depth_attachment_kind_traits = depth_attachment_type::kind_traits;

private:
    UniqueFramebuffer fbo_;
    Size2I                 resolution_; // Primary resolution of the RenderTarget.
    GLsizei                num_array_elements_; // Ignored when `!is_array`.

    depth_attachment_type  depth_;  // Depth or DepthStencil attachment.
    color_attachments_type colors_;

    std::array<bool, num_color_attachments> enabled_{ detail::make_array_filled_with<bool, num_color_attachments>(false) };
    GLuint                                  enabled_for_read_{ 0 };

    struct PrivateKey {};

    RenderTarget(
        PrivateKey           private_key [[maybe_unused]],
        const Size2I&        resolution,
        GLsizei              num_array_elements,
        DepthAttachmentT     depth,
        ColorAttachmentTs... colors) noexcept
        : resolution_        { resolution           }
        , num_array_elements_{ num_array_elements   }
        , depth_             { std::move(depth)     }
        , colors_            { std::move(colors)... }
    {

        update_size_all();
        attach_all();

        enable_all_color_buffers_for_draw();
    }

public:
    RenderTarget(
        const Size2I&        resolution,
        DepthAttachmentT     depth,
        ColorAttachmentTs... colors) noexcept
            requires (!is_array) && has_depth_attachment
        : RenderTarget(PrivateKey{}, resolution, 1, std::move(depth), std::move(colors)...)
    {}

    RenderTarget(
        const Size2I&        resolution,
        ColorAttachmentTs... colors) noexcept
            requires (!is_array) && (!has_depth_attachment)
        : RenderTarget(PrivateKey{}, resolution, 1, {}, std::move(colors)...)
    {}

    RenderTarget(
        const Size2I&        resolution,
        GLsizei              num_array_elements,
        DepthAttachmentT     depth,
        ColorAttachmentTs... colors) noexcept
            requires is_array && has_depth_attachment
        : RenderTarget(PrivateKey{}, resolution, num_array_elements, std::move(depth), std::move(colors)...)
    {}

    RenderTarget(
        const Size2I&        resolution,
        GLsizei              num_array_elements,
        ColorAttachmentTs... colors) noexcept
            requires is_array && (!has_depth_attachment)
        : RenderTarget(PrivateKey{}, resolution, num_array_elements, {}, std::move(colors)...)
    {}



    // TODO: generate_mipmaps()

    // Basic read-only access for only god knows what.
    auto framebuffer() const noexcept
        -> RawFramebuffer<GLConst>
    {
        return fbo_;
    }

    [[nodiscard("BindTokens have to be provided to API calls that expect bound state.")]]
    BindToken<Binding::DrawFramebuffer> bind_draw() noexcept {
        return fbo_->bind_draw();
    }

    [[nodiscard("BindTokens have to be provided to API calls that expect bound state.")]]
    BindToken<Binding::ReadFramebuffer> bind_read() const noexcept {
        return fbo_->bind_read();
    }

    // Blit provided as part of RenderTarget, since there's no way to access the underlying FBO as mutable otherwise.
    template<of_kind<GLKind::Framebuffer, GLKind::DefaultFramebuffer> SrcFB>
    void blit_from(
        const SrcFB&    src_framebuffer,
        const Region2I& src_region,
        const Region2I& dst_region,
        BufferMask      buffers,
        BlitFilter      filter) noexcept
    {
        decay_to_raw(src_framebuffer).blit_to(fbo_, src_region, dst_region, buffers, filter);
    }



    auto depth_attachment() const noexcept
        -> const depth_attachment_type&
            requires has_depth_attachment
    {
        return depth_;
    }

    // Get color attachment by integral Idx or the underlying value of an enum.
    template<auto Idx = size_t{ 0 }>
        requires color_attachment_index_in_bounds<Idx, num_color_attachments>
    auto color_attachment() const noexcept
        -> const color_attachment_type<Idx>&
    {
        return get_color<Idx>();
    }



    auto share_depth_attachment() noexcept
        requires
            has_depth_attachment &&
            (depth_attachment_type::attachment_kind == AttachmentKind::Shareable)
    {
        return depth_.share();
    }

    template<auto Idx>
    auto share_color_attachment() noexcept
        requires
            color_attachment_index_in_bounds<Idx, num_color_attachments> &&
            (color_attachment_type<Idx>::attachment_kind == AttachmentKind::Shareable)
    {
        return get_color<Idx>().share();
    }



    auto resolution() const noexcept
        -> Size2I
    {
        return resolution_;
    }

    auto num_array_elements() const noexcept
        -> GLsizei
            requires is_array
    {
        return num_array_elements_;
    }

    void resize(const Size2I& resolution) noexcept {
        resolution_ = resolution;
        update_size_all();
        attach_all();
    }

    void resize(const Size2I& resolution, GLsizei new_array_elements) noexcept
        requires is_array
    {
        resolution_         = resolution;
        num_array_elements_ = new_array_elements;
        update_size_all();
        attach_all();
    }

    void resize(GLsizei new_array_elements) noexcept
        requires is_array
    {
        num_array_elements_ = new_array_elements;
        update_size_all();
        attach_all();
    }




    void enable_all_color_buffers_for_draw() noexcept {
        std::ranges::fill(enabled_, true);
        update_enabled_draw_buffers();
    }

    // This resets the state on all previously enabled color buffers.
    template<auto ...Ids>
        requires (color_attachment_index_in_bounds<Ids, num_color_attachments> && ...)
    void specify_color_buffers_for_draw() noexcept {
        std::ranges::fill(enabled_, false);
        ((enabled_[Ids] = true), ...);
    }

    template<auto Idx>
        requires color_attachment_index_in_bounds<Idx, num_color_attachments>
    bool is_color_buffer_enabled_for_draw() const noexcept {
        return enabled_[to_underlying_or_value(Idx)];
    }




    template<auto Idx>
        requires color_attachment_index_in_bounds<Idx, num_color_attachments>
    void specify_color_buffer_for_read() noexcept {
        enabled_for_read_ = to_underlying_or_value(Idx);
        fbo_->specify_color_buffer_for_read(enabled_for_read_);
    }

    template<auto Idx>
        requires color_attachment_index_in_bounds<Idx, num_color_attachments>
    bool is_color_buffer_enabled_for_read() const noexcept {
        return to_underlying_or_value(Idx) == enabled_for_read_;
    }




    auto reset_depth_attachment(depth_attachment_type new_depth) noexcept
        -> depth_attachment_type
            requires has_depth_attachment
    {
        depth_attachment_type old_depth{ std::exchange(depth_, std::move(new_depth)) };

        // It is important to update size first, and only attach after that,
        // as we attach by id and that will change on reallocation of the texture.
        // So we want to attach the newly allocated texture, not the old one.
        update_depth_size();
        attach_depth_or_depth_stencil();

        return old_depth;
    }

    template<auto Idx = size_t{ 0 }>
        requires color_attachment_index_in_bounds<Idx, num_color_attachments>
    auto reset_color_attachment(color_attachment_type<Idx> new_color) noexcept
        -> color_attachment_type<Idx>
    {
        color_attachment_type<Idx> old_color{ std::exchange(get_color<Idx>(), std::move(new_color)) };

        update_color_size<Idx>();
        attach_color<Idx>();

        return old_color;
    }



    // TODO: Combined reset_and_resize functions? Combinatorial explosion of overloads, jees.

private:
    // Wrapper of std::get<Id> with conversion to underlying value.
    template<auto AttachmentId>
    auto& get_color() noexcept {
        return std::get<to_underlying_or_value(AttachmentId)>(colors_);
    }
    template<auto AttachmentId>
    const auto& get_color() const noexcept {
        return std::get<to_underlying_or_value(AttachmentId)>(colors_);
    }



    void attach_depth_or_depth_stencil() noexcept {
        InternalFormat iformat = depth_.internal_format();

        auto attach_depth = [this] {
            depth_.attach_as_depth_to(fbo_);
        };

        auto attach_depth_and_stencil = [this] {
            depth_.attach_as_depth_to(fbo_);
            depth_.attach_as_stencil_to(fbo_);
        };

        switch (iformat) {
            case InternalFormat::DepthComponent:
            case InternalFormat::DepthComponent16:
            case InternalFormat::DepthComponent24:
            case InternalFormat::DepthComponent32:
            case InternalFormat::DepthComponent32F:
                attach_depth();
                break;
            case InternalFormat::DepthStencil:
            case InternalFormat::Depth24_Stencil8:
            case InternalFormat::Depth32F_Stencil8:
                attach_depth_and_stencil();
                break;
            default:
                assert(false && "Invalid InternalFormat for Depth or DepthStencil attachment.");
        }
    }

    template<auto Idx>
    void attach_color() noexcept {
        get_color<Idx>().attach_as_color_to(fbo_, to_underlying_or_value(Idx));
    }

    void attach_all_colors() noexcept {
        [this]<size_t ...Idx>(std::index_sequence<Idx...>) {
            (this->attach_color<Idx>(), ...);
        }(std::make_index_sequence<num_color_attachments>());
    }


    void attach_all() noexcept {
        if constexpr (has_depth_attachment) {
            attach_depth_or_depth_stencil();
        }
        attach_all_colors();
    }


    void update_depth_size() noexcept
        requires has_depth_attachment
    {
        // Resize happens only if this RenderTarget is a full owner of an attachment.
        if constexpr (depth_attachment_kind_traits::is_full_owner) {
            if constexpr (depth_attachment_traits::is_array) {
                depth_.resize(resolution_, num_array_elements_);
            } else {
                depth_.resize(resolution_);
            }
        }
    }

    template<auto Idx>
    void update_color_size() noexcept {
        // Resize happens only if this RenderTarget is a full owner of an attachment.
        if constexpr (color_attachment_kind_traits<Idx>::is_full_owner) {
            if constexpr (color_attachment_traits<Idx>::is_array) {
                get_color<Idx>().resize(resolution_, num_array_elements_);
            } else {
                get_color<Idx>().resize(resolution_);
            }
        }
    }


    void update_size_all() noexcept {
        if constexpr (has_depth_attachment) {
            update_depth_size();
        }
        [this]<size_t ...Idx>(std::index_sequence<Idx...>) {
            (this->update_color_size<Idx>(), ...);
        }(std::make_index_sequence<num_color_attachments>());
    }




    void update_enabled_draw_buffers() noexcept {
        std::array<GLenum, num_color_attachments> enabled_constants;
        // My brain is rotting. This could be a normal loop...
        [&, this]<size_t ...Idx>(std::index_sequence<Idx...>) {
            ((enabled_constants[Idx] = enabled_[Idx] ? gl::GL_COLOR_ATTACHMENT0 + Idx : gl::GL_NONE), ...);
        }(std::make_index_sequence<num_color_attachments>());

        fbo_->specify_color_buffers_for_draw(enabled_constants);
    }

};




template<typename DepthAttachmentT, typename ...ColorAttachmentTs>
constexpr bool RenderTarget<DepthAttachmentT, ColorAttachmentTs...>::
    is_any_attachment_multisample() noexcept
{
    bool is_any_ms = (ColorAttachmentTs::texture_traits::is_multisample || ...);
    if constexpr (has_depth_attachment) {
        is_any_ms = is_any_ms || DepthAttachmentT::texture_traits::is_multisample;
    }
    return is_any_ms;
}

template<typename DepthAttachmentT, typename ...ColorAttachmentTs>
constexpr bool RenderTarget<DepthAttachmentT, ColorAttachmentTs...>::
    is_any_attachment_array() noexcept
{
    bool is_any_array = (ColorAttachmentTs::texture_traits::is_array || ...);
    if constexpr (has_depth_attachment) {
        is_any_array = is_any_array || DepthAttachmentT::texture_traits::is_array;
    }
    return is_any_array;
}



// TODO: We need to integrate this into the engine and see how it feels.
// I'm sick of guessing.




} // namespace josh
