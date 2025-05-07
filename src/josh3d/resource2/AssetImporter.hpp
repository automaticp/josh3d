#pragma once
#include "AsyncCradle.hpp"
#include "Coroutines.hpp"
#include "Filesystem.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceFiles.hpp"
#include "TaskCounterGuard.hpp"
#include "UUID.hpp"


namespace josh {


struct ImportModelParams {
    TextureFile::StorageFormat texture_storage_format = TextureFile::StorageFormat::PNG;
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


class AssetImporterContext;


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
        ResourceDatabase& resource_database,
        AsyncCradleRef    async_cradle);

    [[nodiscard]]
    auto import_model(Path file, ImportModelParams params = {})
        -> Job<UUID>;

    [[nodiscard]]
    auto import_texture(Path file, ImportTextureParams params = {})
        -> Job<UUID>;

private:
    friend AssetImporterContext;
    ResourceDatabase&  resource_database_;
    AsyncCradleRef     cradle_;
};


class AssetImporterContext {
public:
    auto& resource_database()  noexcept { return self_.resource_database_;  }
    auto& thread_pool()        noexcept { return self_.cradle_.loading_pool;       }
    auto& offscreen_context()  noexcept { return self_.cradle_.offscreen_context;  }
    auto& completion_context() noexcept { return self_.cradle_.completion_context; }
    auto& local_context()      noexcept { return self_.cradle_.local_context;      }

    // TODO: Remove.
    auto child_context() const noexcept
        -> AssetImporterContext
    {
        return { self_ };
    }
private:
    friend AssetImporter;
    AssetImporterContext(AssetImporter& self) : self_(self), task_guard_(self.cradle_.task_counter) {}
    AssetImporter&  self_;
    SingleTaskGuard task_guard_;
};


} // namespace josh
