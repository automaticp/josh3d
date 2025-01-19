#pragma once
#include "CompletionContext.hpp"
#include "Coroutines.hpp"
#include "Filesystem.hpp"
#include "LocalContext.hpp"
#include "OffscreenContext.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceFiles.hpp"
#include "UUID.hpp"
#include "TaskCounterGuard.hpp"
#include "ThreadPool.hpp"


namespace josh {


struct ImportModelParams {
    // bool skip_meshes     = false;
    // bool skip_textures   = false;
    // bool skip_skeletons  = false;
    // bool skip_animations = false;
    bool collapse_graph   = false; // Equivalent to aiProcess_OptimizeGraph
    bool merge_meshes     = false; // Equivalent to aiProcess_OptimizeMeshes
};


struct ImportTextureParams {
    TextureFile::StorageFormat storage_format; // TODO: Only RAW is supported for now. Ohwell...
    // bool generate_mips = false; // TODO: Not supported yet.
};


/*
AssetImporter is a relatively independent tool that takes
external assets of different kinds (models, meshes, textures, etc.),
converts them into the internal format according to their resource file
spec and stores the references to them in the ResourceDatabase.

This is about "preparing" the assets for runtime loading, not about
the loading itself. Only imported resources can be loaded by the engine.

NOTE: Technically unrelated to the "assimp" library, although
we currently use it internally to import mesh and model data.
*/
class AssetImporter {
public:
    AssetImporter(
        ResourceDatabase&  resource_database,
        ThreadPool&        loading_pool, // Best to use a separate pool for this.
        OffscreenContext&  offscreen_context,
        CompletionContext& completion_context);

    // Must be called periodically from the main thread.
    void update();

    [[nodiscard]]
    auto import_model(Path file, ImportModelParams params = {})
        -> Job<UUID>;

    [[nodiscard]]
    auto import_texture(Path file, ImportTextureParams params = {})
        -> Job<UUID>;

    class Access; // Trying not to leak impl details, but still use importer state.

private:
    ResourceDatabase&  resource_database_;
    ThreadPool&        thread_pool_;
    OffscreenContext&  offscreen_context_;
    CompletionContext& completion_context_;
    TaskCounterGuard   task_counter_;
    LocalContext       local_context_{ task_counter_ };

};


} // namespace josh
