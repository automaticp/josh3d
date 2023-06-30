#pragma once
#include "GBuffer.hpp"
#include "RenderEngine.hpp"
#include "SharedStorage.hpp"
#include "GLScalars.hpp"
#include <glbinding/gl/gl.h>
#include <entt/entity/fwd.hpp>
#include <glbinding/gl/types.h>




namespace learn {


/*
Provides the storage for the GBuffer and clears it on each pass.

Place it before any other stages that draw into the GBuffer.
*/
class GBufferStage {
private:
    SharedStorage<GBuffer> gbuffer_;

public:
    GBufferStage(GLsizei width, GLsizei height)
        : gbuffer_{ width, height }
    {}

    SharedStorageMutableView<GBuffer> get_write_view() noexcept {
        return gbuffer_.share_mutable_view();
    }

    SharedStorageView<GBuffer> get_read_view() const noexcept {
        return gbuffer_.share_view();
    }


    void reset_size(GLsizei width, GLsizei height) {
        gbuffer_->reset_size(width, height);
    }


    void operator()(const RenderEnginePrimaryInterface& engine,
        const entt::registry&)
    {
        using namespace gl;

        if (engine.window_size().width != gbuffer_->width() ||
            engine.window_size().height != gbuffer_->height())
        {
            auto [w, h] = engine.window_size();
            reset_size(w, h);
        }

        gbuffer_->framebuffer().bind_draw()
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








class GBufferStageImGuiHook {
private:
    GBufferStage& stage_;
    SharedStorageView<GBuffer> gbuffer_;

public:
    GBufferStageImGuiHook(GBufferStage& stage)
        : stage_{ stage }
        , gbuffer_{ stage_.get_read_view() }
    {}

    void operator()();
};







} // namespace learn
