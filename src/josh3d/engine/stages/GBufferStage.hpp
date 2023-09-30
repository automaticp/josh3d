#pragma once
#include "GLFramebuffer.hpp"
#include "GLTextures.hpp"
#include "RenderEngine.hpp"
#include "RenderTarget.hpp"
#include "Attachments.hpp"
#include "SharedStorage.hpp"
#include "GLScalars.hpp"
#include <glbinding/gl/enum.h>
#include <glbinding/gl/gl.h>
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/types.h>




namespace josh {


class GBuffer {
public:
    enum class Slot : size_t {
        position_draw = 0,
        normals       = 1,
        albedo_spec   = 2
    };

private:
    using Target = RenderTarget<
        ViewAttachment<RawTexture2D>,   // Depth
        UniqueAttachment<RawTexture2D>, // Position/Draw
        UniqueAttachment<RawTexture2D>, // Normals
        UniqueAttachment<RawTexture2D>  // Albedo/Spec
    >;

    Texture2D depth_;
    Target tgt_;

public:
    GBuffer(
        const Size2I& size,
        const ViewAttachment<RawTexture2D>& depth)
        : tgt_{
            depth,
            { size, { gl::GL_RGBA16F,     gl::GL_RGBA, gl::GL_FLOAT } },
            { size, { gl::GL_RGBA8_SNORM, gl::GL_RGBA, gl::GL_FLOAT } },
            { size, { gl::GL_RGBA8,       gl::GL_RGBA, gl::GL_UNSIGNED_BYTE } }
        }
    {
        using enum GLenum;
        tgt_.color_attachment<Slot::position_draw>().texture().bind()
            .set_min_mag_filters(GL_NEAREST, GL_NEAREST);

        tgt_.color_attachment<Slot::normals>().texture().bind()
            .set_min_mag_filters(GL_NEAREST, GL_NEAREST);

        tgt_.color_attachment<Slot::albedo_spec>().texture().bind()
            .set_min_mag_filters(GL_NEAREST, GL_NEAREST);
    }

    GBuffer(const Size2I& size)
        : GBuffer{
            size,
            { this->depth_, size, { gl::GL_DEPTH_COMPONENT32F, gl::GL_DEPTH_COMPONENT, gl::GL_FLOAT } }
        }
    {
        using enum GLenum;
        depth_.bind()
            .set_min_mag_filters(GL_NEAREST, GL_NEAREST);
    }

    BoundDrawFramebuffer<GLMutable> bind_draw() noexcept { return tgt_.bind_draw(); }
    BoundReadFramebuffer<GLMutable> bind_read() noexcept { return tgt_.bind_read(); }
    BoundReadFramebuffer<GLConst>   bind_read() const noexcept { return tgt_.bind_read(); }


    RawTexture2D<GLConst> depth_texture() const noexcept {
        return tgt_.depth_attachment().texture();
    }

    RawTexture2D<GLConst> position_draw_texture() const noexcept {
        return tgt_.color_attachment<Slot::position_draw>().texture();
    }

    RawTexture2D<GLConst> normals_texture() const noexcept {
        return tgt_.color_attachment<Slot::normals>().texture();
    }

    RawTexture2D<GLConst> albedo_spec_texture() const noexcept {
        return tgt_.color_attachment<Slot::albedo_spec>().texture();
    }

    Size2I size() const noexcept {
        return tgt_.color_attachment<Slot::position_draw>().size();
    }

    void resize(const Size2I& new_size) {
        tgt_.resize_all(new_size);
    }

};





/*
Provides the storage for the GBuffer and clears it on each pass.

Place it before any other stages that draw into the GBuffer.
*/
class GBufferStage {
private:
    SharedStorage<GBuffer> gbuffer_;

public:
    GBufferStage(const Size2I& size)
        : gbuffer_{ size }
    {}

    GBufferStage(const Size2I& size, const ViewAttachment<RawTexture2D>& depth)
        : gbuffer_{ size, depth }
    {}

    SharedStorageMutableView<GBuffer> get_write_view() noexcept {
        return gbuffer_.share_mutable_view();
    }

    SharedStorageView<GBuffer> get_read_view() const noexcept {
        return gbuffer_.share_view();
    }


    void reset_size(const Size2I& new_size) {
        gbuffer_->resize(new_size);
    }


    void operator()(const RenderEnginePrimaryInterface& engine,
        const entt::registry&)
    {
        using namespace gl;

        if (engine.window_size() != gbuffer_->size()) {
            reset_size(engine.window_size());
        }

        gbuffer_->bind_draw()
            .and_then([] {
                // We use alpha of one of the channels in the GBuffer
                // to detect draws made in the deferred stage and properly
                // compose the deferred pass output with what's already
                // been in the main target before the pass.
                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            });
    }

};





} // namespace josh
