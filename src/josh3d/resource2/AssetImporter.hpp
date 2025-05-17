#pragma once
#include "AsyncCradle.hpp"
#include "Common.hpp"
#include "CommonConcepts.hpp"
#include "Coroutines.hpp"
#include "Filesystem.hpp"
#include "ResourceDatabase.hpp"
#include "TaskCounterGuard.hpp"
#include "TypeInfo.hpp"
#include "UUID.hpp"
#include <fmt/core.h>


namespace josh {


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
        AsyncCradleRef    async_cradle
    )
        : resource_database_(resource_database)
        , cradle_           (async_cradle)
    {}

    template<typename ParamsT,
        of_signature<Job<UUID>(AssetImporterContext, Path, ParamsT)> ImporterF>
    void register_importer(ImporterF&& f);

    template<typename ParamsT>
    auto import_asset(Path path, ParamsT params)
        -> Job<UUID>;

    // TODO: Cannot make import_any() unitil I make a custom Any type
    // that converts to AnyRef. Dreaded encapsulation makes life ass again.
    //
    // TODO: Look at unsafe_any_cast(), maybe enough.

private:
    friend AssetImporterContext;
    ResourceDatabase&  resource_database_;
    AsyncCradleRef     cradle_;

    using key_type      = TypeIndex;
    using unpacker_func = UniqueFunction<Job<UUID>(AssetImporterContext, Path, AnyRef)>;
    using dtable_type   = HashMap<key_type, unpacker_func>;

    dtable_type dispatch_table_;

    auto _import_asset(const key_type& key, Path path, AnyRef params)
        -> Job<UUID>;
};


class AssetImporterContext {
public:
    auto& resource_database()  noexcept { return self_.resource_database_;         }
    auto& thread_pool()        noexcept { return self_.cradle_.loading_pool;       }
    auto& offscreen_context()  noexcept { return self_.cradle_.offscreen_context;  }
    auto& completion_context() noexcept { return self_.cradle_.completion_context; }
    auto& local_context()      noexcept { return self_.cradle_.local_context;      }
    auto& importer()           noexcept { return self_;                            }

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




template<typename ParamsT,
    of_signature<Job<UUID>(AssetImporterContext, Path, ParamsT)> ImporterF>
void AssetImporter::register_importer(ImporterF&& f) {
    const key_type key = typeid(ParamsT);
    auto [it, was_emplaced] = dispatch_table_.try_emplace(
        key,
        [f=FORWARD(f)](AssetImporterContext context, Path path, AnyRef params)
            -> Job<UUID>
        {
            // NOTE: We move the params here because we expect the calling side to
            // pass the reference to a moveable copy of the destination object.
            // Most of the time, the params is some kind of a struct so this move
            // is superfluous at best.
            return f(MOVE(context), MOVE(path), MOVE(params.target_unchecked<ParamsT>()));
        }
    );
    assert(was_emplaced);
}


template<typename ParamsT>
auto AssetImporter::import_asset(Path path, ParamsT params)
    -> Job<UUID>
{
    const key_type key = type_id<ParamsT>();
    return _import_asset(key, MOVE(path), AnyRef(params));
}


} // namespace josh
