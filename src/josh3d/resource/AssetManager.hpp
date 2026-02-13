#pragma once
#include "Asset.hpp"
#include "AssetCache.hpp"
#include "MeshRegistry.hpp"
#include "async/CompletionContext.hpp"
#include "async/CoroCore.hpp"
#include "async/Coroutines.hpp"
#include "async/ThreadPool.hpp"
#include "OffscreenContext.hpp"
#include "async/TaskCounterGuard.hpp"
#include "async/ThreadsafeQueue.hpp"
#include "UniqueFunction.hpp"
#include <chrono>
#include <thread>
#include <type_traits>


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

    // A poor workaround for the fact that waiting for models is impossible
    // on the main thread, since the main thread should call update()
    // in order to complete a mesh upload to the MeshStorage.
    //
    // This periodically calls update() while checking completeness of the job.
    //
    // FIXME: Something better should be done. This is awful.
    void wait_until_ready(
        readyable auto&&         readyable,
        std::chrono::nanoseconds sleep_budget = std::chrono::milliseconds(1));

private:
    /*
    Local executor context that will execute new tasks in update().

    You can reschedule tasks here back from other threads to complete
    them inside the main gl context. This, for example, allows
    you to access the MeshRegistry.
    */
    class LocalContext {
    public:
        void emplace(auto&& task) { tasks_.emplace(FORWARD(task)); }
    private:
        ThreadsafeQueue<UniqueFunction<void()>> tasks_;
        friend class AssetManager;
    };

    AssetCache         cache_;
    ThreadPool&        thread_pool_;
    OffscreenContext&  offscreen_context_;
    CompletionContext& completion_context_;
    LocalContext       local_context_;
    MeshRegistry&      mesh_registry_;
    TaskCounterGuard   task_counter_; // Must be last so that it block all other memvars from destruction.
};




void AssetManager::wait_until_ready(
    readyable auto&&         readyable,
    std::chrono::nanoseconds sleep_budget)
{
    using cpo::is_ready;
    while (not is_ready(readyable)) {
        auto wake_up_point = std::chrono::steady_clock::now() + sleep_budget;
        update();
        std::this_thread::sleep_until(wake_up_point);
    }
}




} // namespace josh
