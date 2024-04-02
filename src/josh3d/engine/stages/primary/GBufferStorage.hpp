#pragma once
#include "EnumUtils.hpp"
#include "GLAPIBinding.hpp"
#include "GLMutability.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "Attachments.hpp"
#include "SharedStorage.hpp"
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
        SharedAttachment<Renderable::Texture2D> depth)
        : target_{
            resolution,
            std::move(depth),
            { position_draw_iformat },
            { normals_iformat       },
            { albedo_spec_iformat   },
            { object_id_iformat     }
        }
    {}

    BindToken<Binding::DrawFramebuffer> bind_draw()       noexcept { return target_.bind_draw(); }
    // TODO: Set active read attachment?
    BindToken<Binding::ReadFramebuffer> bind_read() const noexcept { return target_.bind_read(); }




    auto depth_attachment() const noexcept
        -> const SharedAttachment<Renderable::Texture2D>&
    {
        return target_.depth_attachment();
    }

    void reset_depth_attachment(SharedAttachment<Renderable::Texture2D> shared) noexcept {
        target_.reset_depth_attachment(std::move(shared));
    }

    dsa::RawTexture2D<GLConst> depth_texture() const noexcept         { return target_.depth_attachment().texture();                      }
    dsa::RawTexture2D<GLConst> position_draw_texture() const noexcept { return target_.color_attachment<Slot::PositionDraw>().texture(); }
    dsa::RawTexture2D<GLConst> normals_texture() const noexcept       { return target_.color_attachment<Slot::Normals>().texture();       }
    dsa::RawTexture2D<GLConst> albedo_spec_texture() const noexcept   { return target_.color_attachment<Slot::AlbedoSpec>().texture();   }
    dsa::RawTexture2D<GLConst> object_id_texture() const noexcept     { return target_.color_attachment<Slot::ObjectID>().texture();     }

    Size2I resolution() const noexcept { return target_.resolution(); }
    void resize(const Size2I& new_resolution) { target_.resize(new_resolution); }


private:
    using Target = RenderTarget<
        SharedAttachment<Renderable::Texture2D>, // Depth
        UniqueAttachment<Renderable::Texture2D>, // Position/Draw
        UniqueAttachment<Renderable::Texture2D>, // Normals
        UniqueAttachment<Renderable::Texture2D>, // Albedo/Spec
        UniqueAttachment<Renderable::Texture2D>  // ObjectID (TODO: Should be separable)
    >;

    Target target_;

    static constexpr auto position_draw_iformat = InternalFormat::RGBA16F;     // TODO: Should not exist.
    static constexpr auto normals_iformat       = InternalFormat::RGB8_SNorm;  // TODO: Can be encoded in 2 values.
    static constexpr auto albedo_spec_iformat   = InternalFormat::RGBA8;       // TODO: Shininess?
    static constexpr auto object_id_iformat     = InternalFormat::R32UI;

};


} // namespace josh




namespace josh::stages::primary {


/*
Provides the storage for the GBuffer and clears it on each pass.

Place it before any other stages that draw into the GBuffer.
*/
class GBufferStorage {
public:
    GBufferStorage(const Size2I& resolution, SharedAttachment<Renderable::Texture2D> depth)
        : gbuffer_{ resolution, std::move(depth) }
    {}

    SharedStorageMutableView<GBuffer> share_write_view() noexcept {
        return gbuffer_.share_mutable_view();
    }

    SharedStorageView<GBuffer> share_read_view() const noexcept {
        return gbuffer_.share_view();
    }

    const GBuffer& view_gbuffer() const noexcept {
        return *gbuffer_;
    }


    void resize(const Size2I& new_resolution) {
        gbuffer_->resize(new_resolution);
    }


    void operator()(RenderEnginePrimaryInterface& engine) {

        resize(engine.main_resolution());

        if (!engine.main_depth_attachment().is_shared_to(gbuffer_->depth_attachment())) {
            gbuffer_->reset_depth_attachment(engine.share_main_depth_attachment());
        }



        // The ObjectID buffer is cleared with the null sentinel value.
        constexpr entt::id_type null_color{ entt::null };

        auto bound_fbo = gbuffer_->bind_draw();

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
        glapi::clear_color_buffer(bound_fbo, to_underlying(GBuffer::Slot::ObjectID),     RGBAUI{ .r=null_color });

        bound_fbo.unbind();
    }


private:
    SharedStorage<GBuffer> gbuffer_;

};


} // namespace josh::stages::primary
