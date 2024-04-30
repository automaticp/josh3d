#pragma once
#include "EnumUtils.hpp"
#include "GLAPIBinding.hpp"
#include "GLMutability.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "Attachments.hpp"
#include "SharedStorage.hpp"
#include "stages/primary/IDBufferStorage.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glbinding/gl/gl.h>
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/types.h>




namespace josh {


class GBuffer {
public:
    enum class Slot {
        PositionDraw = 0,
        Normals      = 1,
        AlbedoSpec   = 2,
        ObjectID     = 3
    };

    GBuffer(
        const Size2I&                           resolution,
        SharedAttachment<Renderable::Texture2D> depth,
        SharedAttachment<Renderable::Texture2D> object_id)
        : target_{
            resolution,
            std::move(depth),
            { position_draw_iformat },
            { normals_iformat       },
            { albedo_spec_iformat   },
            std::move(object_id)
        }
    {}

    [[nodiscard]] BindToken<Binding::DrawFramebuffer> bind_draw()       noexcept { return target_.bind_draw(); }
    // TODO: Set active read attachment?
    [[nodiscard]] BindToken<Binding::ReadFramebuffer> bind_read() const noexcept { return target_.bind_read(); }




    auto depth_attachment() const noexcept
        -> const SharedAttachment<Renderable::Texture2D>&
    {
        return target_.depth_attachment();
    }

    auto object_id_attachment() const noexcept
        -> const SharedAttachment<Renderable::Texture2D>&
    {
        return target_.color_attachment<Slot::ObjectID>();
    }

    void reset_depth_attachment(SharedAttachment<Renderable::Texture2D> shared) noexcept {
        target_.reset_depth_attachment(std::move(shared));
    }

    void reset_object_id_attachment(SharedAttachment<Renderable::Texture2D> shared) noexcept {
        target_.reset_color_attachment<Slot::ObjectID>(std::move(shared));
    }


    RawTexture2D<GLConst> depth_texture()         const noexcept { return target_.depth_attachment().texture();                     }
    RawTexture2D<GLConst> position_draw_texture() const noexcept { return target_.color_attachment<Slot::PositionDraw>().texture(); }
    RawTexture2D<GLConst> normals_texture()       const noexcept { return target_.color_attachment<Slot::Normals>().texture();      }
    RawTexture2D<GLConst> albedo_spec_texture()   const noexcept { return target_.color_attachment<Slot::AlbedoSpec>().texture();   }
    RawTexture2D<GLConst> object_id_texture()     const noexcept { return target_.color_attachment<Slot::ObjectID>().texture();     }

    Size2I resolution() const noexcept { return target_.resolution(); }
    void resize(const Size2I& new_resolution) { target_.resize(new_resolution); }


private:
    using Target = RenderTarget<
        SharedAttachment<Renderable::Texture2D>, // Depth
        UniqueAttachment<Renderable::Texture2D>, // Position/Draw
        UniqueAttachment<Renderable::Texture2D>, // Normals
        UniqueAttachment<Renderable::Texture2D>, // Albedo/Spec
        SharedAttachment<Renderable::Texture2D>  // ObjectID
    >;

    Target target_;

    static constexpr auto position_draw_iformat = InternalFormat::RGBA16F;     // TODO: Should not exist.
    static constexpr auto normals_iformat       = InternalFormat::RGB8_SNorm;  // TODO: Can be encoded in 2 values.
    static constexpr auto albedo_spec_iformat   = InternalFormat::RGBA8;       // TODO: Shininess?

};


} // namespace josh




namespace josh::stages::primary {


/*
Provides the storage for the GBuffer and clears it on each pass.

Place it before any other stages that draw into the GBuffer.
*/
class GBufferStorage {
public:
    GBufferStorage(
        const Size2I&                           resolution,
        SharedAttachment<Renderable::Texture2D> depth,
        SharedStorageMutableView<IDBuffer>      id_buffer)
        : gbuffer_{
            resolution,
            std::move(depth),
            id_buffer->share_object_id_attachment()
        }
        , idbuffer_{ std::move(id_buffer) }
    {}

    auto share_write_view() noexcept
        -> SharedStorageMutableView<GBuffer>
    {
        return gbuffer_.share_mutable_view();
    }

    auto share_read_view() const noexcept
        -> SharedStorageView<GBuffer>
    {
        return gbuffer_.share_view();
    }

    auto view_gbuffer() const noexcept
        -> const GBuffer&
    {
        return *gbuffer_;
    }


    void resize(const Size2I& new_resolution) {
        gbuffer_->resize(new_resolution);
    }


    void operator()(RenderEnginePrimaryInterface& engine);

private:
    SharedStorage<GBuffer>             gbuffer_;
    SharedStorageMutableView<IDBuffer> idbuffer_;

};




inline void GBufferStorage::operator()(
    RenderEnginePrimaryInterface& engine)
{

    resize(engine.main_resolution());

    if (!gbuffer_->depth_attachment().is_shared_from(engine.main_depth_attachment())) {
        gbuffer_->reset_depth_attachment(engine.share_main_depth_attachment());
    }

    if (!gbuffer_->object_id_attachment().is_shared_from(idbuffer_->object_id_attachment())) {
        gbuffer_->reset_object_id_attachment(idbuffer_->share_object_id_attachment());
    }


    BindGuard bound_fbo{ gbuffer_->bind_draw() };

    // We use alpha of one of the channels in the GBuffer
    // to detect draws made in the deferred stage and properly
    // compose the deferred pass output with what's already
    // been in the main target before the pass.
    //
    // TODO: I am not convinced that above is needed at all.
    // TODO: This entire buffer is not needed tbh. Reconstruct position from depth.
    glapi::clear_color_buffer(bound_fbo, to_underlying(GBuffer::Slot::PositionDraw), RGBAF { 0.f, 0.f, 0.f, 0.f });
    glapi::clear_color_buffer(bound_fbo, to_underlying(GBuffer::Slot::Normals),      RGBAF { 0.f, 0.f, 1.f });
    glapi::clear_color_buffer(bound_fbo, to_underlying(GBuffer::Slot::AlbedoSpec),   RGBAF { 0.f, 0.f, 0.f, 0.f });

}


} // namespace josh::stages::primary
