#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep (concepts)
#include "GLFramebuffer.hpp"
#include "GLMutability.hpp"
#include "GLObjectHelpers.hpp"
#include "GLShared.hpp"
#include "GLTextures.hpp"
// #include "GLRenderbuffer.hpp"
#include "GLObjects.hpp"
#include "GLUnique.hpp"
#include "detail/ConditionalMixin.hpp"
#include "detail/StaticAssertFalseMacro.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <concepts>
#include <type_traits>


namespace josh {



// Dictates how the LOD of the underlying texture will be allocated.
// - NoLOD means only level 0 will be allocated;
// - MaxLOD means it will allocate enough levels such that the last would be 1x1.
enum class LODPolicy {
    NoLOD,
    MaxLOD,
};


enum class AttachmentKind {
    Unique,      // Full owner, controls size and format.
    Shareable,   // Full owner, controls size and format.
    Shared,      // Only owns lifetime, not size or format.
    SharedLayer, // Not implemented yet. Only owns lifetime, not size or format.
};


namespace detail {


// OOOOH TRAIT TABLES HERE WE GO AGAIN
template<AttachmentKind AKindV> struct attachment_kind_full_owner         : std::true_type  {};
template<> struct attachment_kind_full_owner<AttachmentKind::Shared>      : std::false_type {};
template<> struct attachment_kind_full_owner<AttachmentKind::SharedLayer> : std::false_type {};


template<AttachmentKind AKindV> struct attachment_kind_traits {
    static constexpr bool is_full_owner = attachment_kind_full_owner<AKindV>::value;
    static constexpr bool is_layer      = AKindV == AttachmentKind::SharedLayer;
    static constexpr bool is_shareable  = AKindV == AttachmentKind::Shareable;
};



template<template<typename> typename OwnerT, typename TextureT, AttachmentKind AKindV>
struct AttachmentInterface_Core {
public:
    using texture_type           = TextureT;
    using texture_traits         = josh::texture_traits<texture_type>;
    using kind_traits            = attachment_kind_traits<AKindV>;
    using resolution_type        = texture_traits::resolution_type;

    static constexpr size_t resolution_ndims = texture_traits::resolution_ndims;
    static constexpr AttachmentKind attachment_kind = AKindV;


protected:
    using mt = mutability_traits<texture_type>;

    static auto zero_resolution() noexcept {
        if constexpr      (resolution_ndims == 1) { return Size1I{ 0 };       }
        else if constexpr (resolution_ndims == 2) { return Size2I{ 0, 0 };    }
        else if constexpr (resolution_ndims == 3) { return Size3I{ 0, 0, 0 }; }
    }


    OwnerT<texture_type> texture_{};
    InternalFormat       iformat_;
    resolution_type      resolution_{ zero_resolution() };

    // Initialization constructor for full owners.
    AttachmentInterface_Core(InternalFormat iformat) noexcept
            requires attachment_kind_traits<AKindV>::is_full_owner
        : iformat_{ iformat }
    {}

    // Replacement constructor for sharing ownership across types of attachments.
    AttachmentInterface_Core(
        OwnerT<texture_type>   texture,
        InternalFormat         iformat,
        const resolution_type& resolution) noexcept
            requires (!attachment_kind_traits<AKindV>::is_full_owner)
        : texture_   { std::move(texture) }
        , iformat_   { iformat            }
        , resolution_{ resolution         }
    {}

public:
    InternalFormat      internal_format() const noexcept { return iformat_;    }
    resolution_type     resolution()      const noexcept { return resolution_; }
    mt::const_type      texture()         const noexcept { return texture_;    }
};





template<AttachmentKind AKindV>
struct AttachmentInterface_Array {
protected:
    GLsizei num_array_elements_{};

    // Initialization constructor for full owners.
    AttachmentInterface_Array()
        requires attachment_kind_traits<AKindV>::is_full_owner
    = default;

    // Replacement constructor for sharing ownership across types of attachments.
    AttachmentInterface_Array(GLsizei num_array_elements) noexcept
            requires (!attachment_kind_traits<AKindV>::is_full_owner)
        : num_array_elements_{ num_array_elements }
    {}
public:
    GLsizei num_array_elements() const noexcept { return num_array_elements_; }
};




struct AttachmentInterface_Multisample {
protected:
    NumSamples      num_samples_      = NumSamples{ 1 };
    SampleLocations sample_locations_ = SampleLocations::NotFixed;

    AttachmentInterface_Multisample(NumSamples num_samples, SampleLocations sample_locations)
        : num_samples_(num_samples), sample_locations_(sample_locations)
    {}
public:
    NumSamples      num_samples()      const noexcept { return num_samples_;      }
    SampleLocations sample_locations() const noexcept { return sample_locations_; }
};




template<AttachmentKind AKindV>
struct AttachmentInterface_LOD {
protected:
    LODPolicy lod_policy_;
    NumLevels num_levels_{ 0 }; // No storage -> No levels

    // Initialization constructor for full owners.
    AttachmentInterface_LOD(LODPolicy lod_policy) noexcept
            requires attachment_kind_traits<AKindV>::is_full_owner
        : lod_policy_{ lod_policy }
    {}

    // Replacement constructor for sharing ownership across types of attachments.
    AttachmentInterface_LOD(LODPolicy lod_policy, NumLevels num_levels) noexcept
            requires (!attachment_kind_traits<AKindV>::is_full_owner)
        : lod_policy_{ lod_policy }
        , num_levels_{ num_levels }
    {}
public:
    LODPolicy lod_policy() const noexcept { return lod_policy_; }
    NumLevels num_levels() const noexcept { return num_levels_; }
};




struct AttachmentInterface_Layer {
protected:
    Layer layer_;

    AttachmentInterface_Layer(Layer layer) noexcept
        : layer_{ layer }
    {}
public:
    Layer layer() const noexcept { return layer_; }
};



template<
    typename CRTP,
    template<typename> typename OwnerT,
    typename TextureT,
    AttachmentKind AKindV
>
struct AttachmentInterface
    : AttachmentInterface_Core<OwnerT, TextureT, AKindV>
    , conditional_mixin_t<
        texture_traits<TextureT>::has_lod,
        AttachmentInterface_LOD<AKindV>
    >
    , conditional_mixin_t<
        texture_traits<TextureT>::is_array,
        AttachmentInterface_Array<AKindV>
    >
    , conditional_mixin_t<
        texture_traits<TextureT>::is_multisample,
        AttachmentInterface_Multisample
    >
    , conditional_mixin_t<
        attachment_kind_traits<AKindV>::is_layer,
        AttachmentInterface_Layer
    >
{
private:
    CRTP& self() noexcept { return static_cast<CRTP&>(*this); }
    using tt = texture_traits<TextureT>;
    using at = attachment_kind_traits<AKindV>;
    using core_type        = AttachmentInterface_Core<OwnerT, TextureT, AKindV>;
    using lod_type         = AttachmentInterface_LOD<AKindV>;
    using array_type       = AttachmentInterface_Array<AKindV>;
    using multisample_type = AttachmentInterface_Multisample;
    using layer_type       = AttachmentInterface_Layer;
public:

    AttachmentInterface(InternalFormat iformat)
        requires at::is_full_owner && (!tt::is_multisample) && (!tt::has_lod)
            : core_type(iformat)
    {}

    AttachmentInterface(InternalFormat iformat, LODPolicy lod_policy = LODPolicy::NoLOD)
        requires at::is_full_owner && (!tt::is_multisample) && tt::has_lod
            : core_type(iformat)
            , lod_type(lod_policy)
    {}

    AttachmentInterface(
        InternalFormat  iformat,
        NumSamples      num_samples,
        SampleLocations sample_locations)
            requires at::is_full_owner && tt::is_multisample
        : core_type(iformat)
        , multisample_type(num_samples, sample_locations)
    {}

protected:
    // Replacement constructors for shared types.

    // TODO: This desperately needs some kind of "builder".
    // How the f. do you synthesize a constructor at compile time?

    // Not LOD and Not MS.

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution)
            requires (!at::is_full_owner) && (!tt::is_multisample) && (!tt::has_lod) && (!tt::is_array)
        : core_type(std::move(texture), iformat, resolution)
    {}

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        GLsizei          num_array_elements)
            requires (!at::is_full_owner) && (!tt::is_multisample) && (!tt::has_lod) && tt::is_array
        : core_type(std::move(texture), iformat, resolution)
        , array_type(num_array_elements)
    {}

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        Layer            layer)
            requires (!at::is_full_owner) && at::is_layer && (!tt::is_multisample) && (!tt::has_lod) && (!tt::is_array)
        : core_type(std::move(texture), iformat, resolution)
        , layer_type(layer)
    {}

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        GLsizei          num_array_elements,
        Layer            layer)
            requires (!at::is_full_owner) && at::is_layer && (!tt::is_multisample) && (!tt::has_lod) && tt::is_array
        : core_type(std::move(texture), iformat, resolution)
        , array_type(num_array_elements)
        , layer_type(layer)
    {}


    // Has LOD.

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        LODPolicy        lod_policy,
        NumLevels        num_levels)
            requires (!at::is_full_owner) && tt::has_lod && (!tt::is_array)
        : core_type(std::move(texture), iformat, resolution)
        , lod_type(lod_policy, num_levels)
    {}

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        GLsizei          num_array_elements,
        LODPolicy        lod_policy,
        NumLevels        num_levels)
            requires (!at::is_full_owner) && tt::has_lod && tt::is_array
        : core_type(std::move(texture), iformat, resolution)
        , array_type(num_array_elements)
        , lod_type(lod_policy, num_levels)
    {}

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        Layer            layer,
        LODPolicy        lod_policy,
        NumLevels        num_levels)
            requires (!at::is_full_owner) && at::is_layer && tt::has_lod && (!tt::is_array)
        : core_type(std::move(texture), iformat, resolution)
        , layer_type(layer)
        , lod_type(lod_policy, num_levels)
    {}

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        GLsizei          num_array_elements,
        Layer            layer,
        LODPolicy        lod_policy,
        NumLevels        num_levels)
            requires (!at::is_full_owner) && at::is_layer && tt::has_lod && tt::is_array
        : core_type(std::move(texture), iformat, resolution)
        , array_type(num_array_elements)
        , layer_type(layer)
        , lod_type(lod_policy, num_levels)
    {}


    // MS.

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        NumSamples       num_samples,
        SampleLocations  sample_locations)
            requires (!at::is_full_owner) && tt::is_multisample && (!tt::is_array)
        : core_type(std::move(texture), iformat, resolution)
        , multisample_type(num_samples, sample_locations)
    {}

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        GLsizei          num_array_elements,
        NumSamples       num_samples,
        SampleLocations  sample_locations)
            requires (!at::is_full_owner) && tt::is_multisample && tt::is_array
        : core_type(std::move(texture), iformat, resolution)
        , array_type(num_array_elements)
        , multisample_type(num_samples, sample_locations)
    {}

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        Layer            layer,
        NumSamples       num_samples,
        SampleLocations  sample_locations)
            requires (!at::is_full_owner) && at::is_layer && tt::is_multisample && (!tt::is_array)
        : core_type(std::move(texture), iformat, resolution)
        , multisample_type(num_samples, sample_locations)
        , layer_type(layer)
    {}

    AttachmentInterface(
        OwnerT<TextureT> texture,
        InternalFormat   iformat,
        const core_type::resolution_type& resolution,
        GLsizei          num_array_elements,
        Layer            layer,
        NumSamples       num_samples,
        SampleLocations  sample_locations)
            requires (!at::is_full_owner) && at::is_layer && tt::is_multisample && tt::is_array
        : core_type(std::move(texture), iformat, resolution)
        , array_type(num_array_elements)
        , multisample_type(num_samples, sample_locations)
        , layer_type(layer)
    {}



private:
    void update_num_levels(const tt::resolution_type& resolution) noexcept
        requires tt::has_lod
    {
        if (self().lod_policy_ == LODPolicy::NoLOD) {

            self().num_levels_ = NumLevels{ 1 };

        } else if (self().lod_policy_ == LODPolicy::MaxLOD) {

            self().num_levels_ = max_num_levels(resolution);

        } else {
            assert(false);
        }
    }

    void reset_texture_if_has_storage() {
        auto resolution = self().texture_->get_resolution();
        if (resolution.width != 0) /* then we have storage allocated */ {
            self().texture_ = OwnerT<TextureT>();
        }
    }
public:

    void resize(const tt::resolution_type& new_resolution) {
        if (self().resolution_ != new_resolution) {


            reset_texture_if_has_storage();


            if constexpr (tt::has_lod) {


                update_num_levels(new_resolution);


                if constexpr (tt::is_array) {

                    self().texture_->allocate_storage(new_resolution, self().num_array_elements_, self().iformat_, self().num_levels_);

                } else /* not array */ {

                    self().texture_->allocate_storage(new_resolution, self().iformat_, self().num_levels_);

                }
            } else if constexpr (tt::is_multisample) {
                if constexpr (tt::is_array) {

                    self().texture_->allocate_storage(
                        new_resolution, self().num_array_elements_, self().iformat_, self().num_samples_, self().sample_locations_);

                } else /* not array */ {

                    self().texture_->allocate_storage(
                        new_resolution, self().iformat_, self().num_samples_, self().sample_locations_);

                }
            } else /* no LOD and not MS (TextureRectangle) */ {

                self().texture_->allocate_storage(new_resolution, self().iformat_);

            }

            // Update resolution.
            self().resolution_ = new_resolution;
        }
    }


    void resize(const tt::resolution_type& new_resolution, GLsizei new_array_elements)
        requires tt::is_array
    {
        // Empty arrays are possible, empty resolution is not.
        if (new_array_elements == 0) {
            reset_texture_if_has_storage();
            self().num_array_elements_ = new_array_elements;
            return;
        }


        if (self().resolution_         != new_resolution ||
            self().num_array_elements_ != new_array_elements)
        {

            reset_texture_if_has_storage();


            if constexpr (tt::has_lod) {

                update_num_levels(new_resolution);

                self().texture_->allocate_storage(
                    new_resolution, new_array_elements, self().iformat_, self().num_levels_);

            } else if constexpr (tt::is_multisample) {

                self().texture_->allocate_storage(
                    new_resolution, new_array_elements, self().iformat_, self().num_samples_, self().sample_locations_);

            } else /* no LOD and not MS (no such array texture) */ {
                JOSH3D_STATIC_ASSERT_FALSE(CRTP);
            }

            // Update resolution and array size.
            self().resolution_         = new_resolution;
            self().num_array_elements_ = new_array_elements;
        }
    }

    void resize(GLsizei new_array_elements)
        requires tt::is_array
    {
        resize(self().resolution(), new_array_elements);
    }



    // This inversion is here so that you could attach to Framebuffers without exposing
    // the mutable version of the underlying texture object.
    void attach_as_stencil_to(RawFramebuffer<GLMutable> fbo) noexcept {
        if constexpr (at::is_layer) {
            fbo.attach_texture_layer_to_stencil_buffer(this->texture_, this->layer_);
        } else {
            fbo.attach_texture_to_stencil_buffer(this->texture_);
        }
    }

    void attach_as_depth_to(RawFramebuffer<GLMutable> fbo) noexcept {
        if constexpr (at::is_layer) {
            fbo.attach_texture_layer_to_depth_buffer(this->texture_, this->layer_);
        } else {
            fbo.attach_texture_to_depth_buffer(this->texture_);
        }

    }

    void attach_as_color_to(RawFramebuffer<GLMutable> fbo, GLuint color_buffer) noexcept {
        if constexpr (at::is_layer) {
            fbo.attach_texture_layer_to_color_buffer(this->texture_, this->layer_, color_buffer);
        } else {
            fbo.attach_texture_to_color_buffer(this->texture_, color_buffer);
        }
    }

};





} // namespace detail




enum class Renderable {
    Texture1DArray,
    TextureRectangle,
    Texture2D,
    Texture2DMS,
    Texture2DArray,
    Texture2DMSArray,
    Cubemap,
    CubemapArray,
    Texture3D,
    Renderbuffer,
    RenderbufferMS,
};


namespace detail {

template<Renderable RenderableV> struct renderable_type;
template<Renderable RenderableV> using  renderable_type_t = renderable_type<RenderableV>::type;

template<> struct renderable_type<Renderable::Texture1DArray>   { using type = RawTexture1DArray<>;   };
template<> struct renderable_type<Renderable::TextureRectangle> { using type = RawTextureRectangle<>; };
template<> struct renderable_type<Renderable::Texture2D>        { using type = RawTexture2D<>;        };
template<> struct renderable_type<Renderable::Texture2DMS>      { using type = RawTexture2DMS<>;      };
template<> struct renderable_type<Renderable::Texture2DArray>   { using type = RawTexture2DArray<>;   };
template<> struct renderable_type<Renderable::Texture2DMSArray> { using type = RawTexture2DMSArray<>; };
template<> struct renderable_type<Renderable::Cubemap>          { using type = RawCubemap<>;          };
template<> struct renderable_type<Renderable::CubemapArray>     { using type = RawCubemapArray<>;     };
template<> struct renderable_type<Renderable::Texture3D>        { using type = RawTexture3D<>;        };
// template<> struct renderable_type<Renderable::Renderbuffer>     { using type = RawRenderbuffer<>;     };
// template<> struct renderable_type<Renderable::RenderbufferMS>   { using type = RawRenderbufferMS<>;   };


template<Renderable RenderableV> struct renderable_traits { using type = texture_traits<renderable_type_t<RenderableV>>; };
template<Renderable RenderableV> using  renderable_traits_t = renderable_traits<RenderableV>::type;


} // namespace detail




/*
Special attachment type used to create a RenderTarget
with no depth or depth/stencil attachment.
*/
struct NoDepthAttachment {
    using texture_traits = void;
    using kind_traits    = void;
};




template<Renderable RenderableV>
class UniqueAttachment
    : public detail::AttachmentInterface<
        UniqueAttachment<RenderableV>,
        GLUnique,
        detail::renderable_type_t<RenderableV>,
        AttachmentKind::Unique
    >
{
private:
    using AttachmentInterface =
        detail::AttachmentInterface<
            UniqueAttachment<RenderableV>,
            GLUnique,
            detail::renderable_type_t<RenderableV>,
            AttachmentKind::Unique
        >;
public:
    using AttachmentInterface::AttachmentInterface;
};




// Shared and Shareable are needed to differentiate between those who control the size.
template<Renderable RenderableV>
class ShareableAttachment;




/*
`Shareable` can be shared, `Shared` cannot. Capische?

No public constructors other than copy/move.
This type must be created by calling `share()` on `ShareableAttachment`.
*/
template<Renderable RenderableV>
class SharedAttachment
    : public detail::AttachmentInterface<
        SharedAttachment<RenderableV>,
        GLShared,
        detail::renderable_type_t<RenderableV>,
        AttachmentKind::Shared
    >
{
private:
    using AttachmentInterface =
        detail::AttachmentInterface<
            SharedAttachment<RenderableV>,
            GLShared,
            detail::renderable_type_t<RenderableV>,
            AttachmentKind::Shared
        >;

    friend ShareableAttachment<RenderableV>;
    using AttachmentInterface::AttachmentInterface;
public:
    bool is_shared_from(const ShareableAttachment<RenderableV>& shareable) const noexcept;
    bool is_shared_with(const SharedAttachment<RenderableV>& shared) const noexcept;
};


/*
Attachment as a shared layer of another.

This type must be created by calling `share_layer()` on `ShareableAttachment`.
*/
template<Renderable RenderableV>
class SharedLayerAttachment
    : public detail::AttachmentInterface<
        SharedLayerAttachment<RenderableV>,
        GLShared,
        detail::renderable_type_t<RenderableV>,
        AttachmentKind::SharedLayer
    >
{
private:
    using AttachmentInterface =
        detail::AttachmentInterface<
            SharedLayerAttachment<RenderableV>,
            GLShared,
            detail::renderable_type_t<RenderableV>,
            AttachmentKind::SharedLayer
        >;

    friend ShareableAttachment<RenderableV>;
    using AttachmentInterface::AttachmentInterface;
public:
    bool is_shared_from(const ShareableAttachment<RenderableV>& shareable) const noexcept;
    bool is_shared_with(const SharedLayerAttachment<RenderableV>& shared) const noexcept;
};




template<Renderable RenderableV>
class ShareableAttachment
    : public detail::AttachmentInterface<
        ShareableAttachment<RenderableV>,
        GLShared,
        detail::renderable_type_t<RenderableV>,
        AttachmentKind::Shareable
    >
{
private:
    using AttachmentInterface =
        detail::AttachmentInterface<
            ShareableAttachment<RenderableV>,
            GLShared,
            detail::renderable_type_t<RenderableV>,
            AttachmentKind::Shareable
        >;
    using rt = detail::renderable_traits_t<RenderableV>;
public:
    using AttachmentInterface::AttachmentInterface;

    ShareableAttachment(const ShareableAttachment& other)            = delete;
    ShareableAttachment& operator=(const ShareableAttachment& other) = delete;
    ShareableAttachment(ShareableAttachment&& other)                 = default;
    ShareableAttachment& operator=(ShareableAttachment&& other)      = default;

    auto share() noexcept
        -> SharedAttachment<RenderableV>
    {
        if constexpr (rt::has_lod) {
            if constexpr (rt::is_array) { return { this->texture_, this->iformat_, this->resolution_, this->num_array_elements_, this->lod_policy_, this->num_levels_ }; }
            else                        { return { this->texture_, this->iformat_, this->resolution_,                            this->lod_policy_, this->num_levels_ }; }
        } else if constexpr (rt::is_multisample) {
            if constexpr (rt::is_array) { return { this->texture_, this->iformat_, this->resolution_, this->num_array_elements_, this->num_samples_, this->sample_locations_ }; }
            else                        { return { this->texture_, this->iformat_, this->resolution_,                            this->num_samples_, this->sample_locations_ }; }
        } else {
            return                               { this->texture_, this->iformat_, this->resolution_, };
        }
    }

    auto share_layer(Layer layer) noexcept
        -> SharedLayerAttachment<RenderableV>
            requires detail::renderable_traits_t<RenderableV>::is_layered
    {
        if constexpr (rt::has_lod) {
            if constexpr (rt::is_array) { return { this->texture_, this->iformat_, this->resolution_, this->num_array_elements_, layer, this->lod_policy_, this->num_levels_ }; }
            else                        { return { this->texture_, this->iformat_, this->resolution_,                            layer, this->lod_policy_, this->num_levels_ }; }
        } else if constexpr (rt::is_multisample) {
            if constexpr (rt::is_array) { return { this->texture_, this->iformat_, this->resolution_, this->num_array_elements_, layer, this->num_samples_, this->sample_locations_ }; }
            else                        { return { this->texture_, this->iformat_, this->resolution_,                            layer, this->num_samples_, this->sample_locations_ }; }
        } else {
            return                               { this->texture_, this->iformat_, this->resolution_,                            layer                                              };
        }
    }

    bool is_shared_to(const SharedAttachment<RenderableV>& shared) const noexcept {
        return shared.texture().id() == this->texture().id();
    }

    bool is_shared_to(const SharedLayerAttachment<RenderableV>& shared) const noexcept {
        return shared.texture().id() == this->texture().id();
    }
};




template<Renderable RenderableV>
inline bool SharedAttachment<RenderableV>::
    is_shared_from(const ShareableAttachment<RenderableV>& shareable) const noexcept
{
    return shareable.is_shared_to(*this);
}

template<Renderable RenderableV>
inline bool SharedAttachment<RenderableV>::
    is_shared_with(const SharedAttachment<RenderableV>& shared) const noexcept
{
    return shared.texture().id() == this->texture().id;
}


template<Renderable RenderableV>
inline bool SharedLayerAttachment<RenderableV>::
    is_shared_from(const ShareableAttachment<RenderableV>& shareable) const noexcept
{
    return shareable.is_shared_to(*this);
}

template<Renderable RenderableV>
inline bool SharedLayerAttachment<RenderableV>::
    is_shared_with(const SharedLayerAttachment<RenderableV>& shared) const noexcept
{
    return shared.texture().id() == this->texture().id;
}






} // namespace josh

