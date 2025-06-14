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
        Normals  = 0,
        Albedo   = 1,
        Specular = 2,
        ObjectID = 3
    };

    // FIXME: This is awful. Stop. Just use a framebuffer.
    GBuffer(
        const Size2I&                           resolution,
        SharedAttachment<Renderable::Texture2D> depth,
        SharedAttachment<Renderable::Texture2D> object_id)
        : target_{
            resolution,
            std::move(depth),
            { normals_iformat  },
            { albedo_iformat   },
            { specular_iformat },
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


    RawTexture2D<GLConst> depth_texture()     const noexcept { return target_.depth_attachment().texture();                 }
    RawTexture2D<GLConst> normals_texture()   const noexcept { return target_.color_attachment<Slot::Normals>().texture();  }
    RawTexture2D<GLConst> albedo_texture()    const noexcept { return target_.color_attachment<Slot::Albedo>().texture();   }
    RawTexture2D<GLConst> specular_texture()  const noexcept { return target_.color_attachment<Slot::Specular>().texture(); }
    RawTexture2D<GLConst> object_id_texture() const noexcept { return target_.color_attachment<Slot::ObjectID>().texture(); }

    Size2I resolution() const noexcept { return target_.resolution(); }
    void resize(const Size2I& new_resolution) { target_.resize(new_resolution); }


private:
    using Target = RenderTarget<
        SharedAttachment<Renderable::Texture2D>, // D. Depth
        UniqueAttachment<Renderable::Texture2D>, // 0. Normals
        UniqueAttachment<Renderable::Texture2D>, // 1. Albedo
        UniqueAttachment<Renderable::Texture2D>, // 2. Specular
        SharedAttachment<Renderable::Texture2D>  // 3. ObjectID
    >;

    Target target_;

    static constexpr auto normals_iformat  = InternalFormat::RGB8_SNorm;
    static constexpr auto albedo_iformat   = InternalFormat::RGB8;
    static constexpr auto specular_iformat = InternalFormat::R8; // TODO: Shininess?


    // using Target2 = RenderTarget<
    //     SharedAttachment<Renderable::Texture2D>, // Depth
    //     UniqueAttachment<Renderable::Texture2D>, // Normals
    //     UniqueAttachment<Renderable::Texture2D>, // Albedo
    //     UniqueAttachment<Renderable::Texture2D>, // Specular
    //     UniqueAttachment<Renderable::Texture2D>, // Shininess
    //     SharedAttachment<Renderable::Texture2D>  // ID
    // >;

    /*
    (Old: 64 + 24 + 32 = 152 bits)


    Depth:     ???         // Use to reconstruct position, test depth < z_far to discard no draw
    Normals:   RGB8_SNorm; // [0, 1] already normalized to 1, need 3 components to tell +Z from -Z vectors
                           // If we were to store view-space normals, then maybe we could get away
                           // with assumption that it is always +Z.
    Albedo:    RGB8;       // [0, 1] rgb values
    Specular:  R8;         // [0, 1] specular factor
    Shininess: R16F;       // [0, inf] phong NDF parameter

    (New: 32 + 24 + 24 + 8 + 16 = 104 bits)

    Emission:  RGB16F // Jeez...

    */




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
        SharedAttachment<Renderable::Texture2D> depth, // FIXME: Don't like this.
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

    if (not gbuffer_->depth_attachment().is_shared_from(engine.main_depth_attachment()))
        gbuffer_->reset_depth_attachment(engine.share_main_depth_attachment());

    if (auto* idbuffer = engine.belt().try_get<IDBuffer>())
        if (not gbuffer_->object_id_attachment().is_shared_from(idbuffer->object_id_attachment()))
            gbuffer_->reset_object_id_attachment(idbuffer->share_object_id_attachment());

    BindGuard bound_fbo{ gbuffer_->bind_draw() };

    glapi::clear_color_buffer(bound_fbo, to_underlying(GBuffer::Slot::Normals),  RGBAF{ 0.f, 0.f, 0.f });
    glapi::clear_color_buffer(bound_fbo, to_underlying(GBuffer::Slot::Albedo),   RGBAF{ 0.f, 0.f, 0.f, 0.f });
    glapi::clear_color_buffer(bound_fbo, to_underlying(GBuffer::Slot::Specular), RGBAF{ 0.f });

    engine.belt().put_ref(*gbuffer_);
}


} // namespace josh::stages::primary
