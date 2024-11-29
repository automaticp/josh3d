#pragma once
#include "Asset.hpp"
#include "AssetCache.hpp"
#include "CompletionContext.hpp"
#include "Coroutines.hpp"
#include "ThreadPool.hpp"
#include "OffscreenContext.hpp"
#include "MeshRegistry.hpp"
#include "TaskCounterGuard.hpp"
#include <cassert>


namespace josh {


class AssetManager {
public:
    AssetManager(
        ThreadPool&        loading_pool, // Best to use a separate pool for this.
        OffscreenContext&  offscreen_context,
        CompletionContext& completion_context,
        MeshRegistry&      mesh_registry);

    // Must be called periodically from the main context.
    void update();

    [[nodiscard]]
    auto load_model(AssetPath path)
        -> Job<SharedModelAsset>;

    [[nodiscard]]
    auto load_texture(AssetPath path, ImageIntent intent)
        -> Job<SharedTextureAsset>;

    [[nodiscard]]
    auto load_cubemap(AssetPath path, CubemapIntent intent)
        -> Job<SharedCubemapAsset>;

    // TODO: Both ImageIntent and CubemapIntent should be part of the
    // key in AssetCache, as the resulting resources cannot be reused
    // across intents.

private:
    AssetCache         cache_;
    ThreadPool&        thread_pool_;
    OffscreenContext&  offscreen_context_;
    CompletionContext& completion_context_;
    MeshRegistry&      mesh_registry_; // TODO: Not supported right now. Need to have a local context for that.
    TaskCounterGuard   task_counter_; // Must be last so that it block all other memvars from destruction.
};


} // namespace josh
