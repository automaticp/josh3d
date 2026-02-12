#include "Runtime.hpp"


namespace josh {

Runtime::Runtime(const RuntimeParams& p)
    : async_cradle(
        p.task_pool_size,
        p.loading_pool_size,
        p.main_window
    )
    , asset_manager(
        async_cradle.loading_pool,
        async_cradle.offscreen_context,
        async_cradle.completion_context,
        mesh_registry
    )
    , asset_unpacker(
        registry
    )
    , scene_importer(
        asset_manager,
        asset_unpacker,
        registry
    )
    , primitives(
        asset_manager
    )
    , resource_database(
        p.database_root
    )
    , asset_importer(
        resource_database,
        async_cradle
    )
    // EWW: Lots of fragmented state.
    , resource_loader(
        resource_database,
        resource_registry,
        mesh_registry,
        async_cradle
    )
    , resource_unpacker(
        resource_database,
        resource_registry,
        resource_loader,
        async_cradle
    )
    , renderer(
        p.main_resolution,
        p.main_format
    )
{}

} // namespace josh
