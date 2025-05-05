#pragma once
#include "CompletionContext.hpp"
#include "ECS.hpp"
#include "LocalContext.hpp"
#include "OffscreenContext.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceRegistry.hpp"
#include "TaskCounterGuard.hpp"
#include "ThreadPool.hpp"


namespace josh {


/*
A class responsible for delivering resources from disk, through
the resource registry, and (possibly) into the final scene.
*/
class ResourceLoader {
public:
    ResourceLoader(
        // TODO: Might be ok to require the callbacks to capture this instead.
        // Not every resource ends up in the scene registry.
        Registry&          scene_registry,
        ResourceDatabase&  resource_database,
        ResourceRegistry&  resource_registry,
        ThreadPool&        loading_pool,
        OffscreenContext&  offscreen_context,
        CompletionContext& completion_context);

    // Must be called periodically from the main thread.
    void update();

    // TODO: Likely could just take a UUID alone. And figure out the type of the resource automatically.
    // There's, however, no way to decide what the "destination" of the loaded resource is.
    // Some can be handles, while others cannot.

    [[nodiscard]]
    auto load_scene(const UUID& uuid, Handle dst_handle)
        -> Job<void>;

    // [[nodiscard]]
    // auto import_model(Path file, ImportModelParams params = {})
    //     -> Job<UUID>;
    // [[nodiscard]]
    // auto import_texture(Path file, ImportTextureParams params = {})
    //     -> Job<UUID>;

    class Access; // Trying not to leak impl details, but still use loader state.

private:
    Registry&          scene_registry_;
    ResourceDatabase&  resource_database_;
    ResourceRegistry&  resource_registry_;
    ThreadPool&        thread_pool_;
    OffscreenContext&  offscreen_context_;
    CompletionContext& completion_context_;
    TaskCounterGuard   task_counter_;
    LocalContext       local_context_{ task_counter_ };
};


} // namespace josh
