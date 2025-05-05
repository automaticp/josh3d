#include "AssetImporter.hpp"
#include "detail/AssetImporter.hpp"
#include "CategoryCasts.hpp"
#include "Coroutines.hpp"
#include "ResourceDatabase.hpp"
#include "UUID.hpp"


/*
NOTE: Most of the implementation has been moved into detail/ and split into multiple files
to preserve compile times and my sanity. Poor clangd would take 3s to respond otherwise.

I blame ranges.
*/
namespace josh {


AssetImporter::AssetImporter(
    ResourceDatabase&  resource_database,
    ThreadPool&        loading_pool,
    OffscreenContext&  offscreen_context,
    CompletionContext& completion_context
)
    : resource_database_ { resource_database  }
    , thread_pool_       { loading_pool       }
    , offscreen_context_ { offscreen_context  }
    , completion_context_{ completion_context }
{}


void AssetImporter::update() {
    while (std::optional task = local_context_.tasks.try_pop()) {
        (*task)();
    }
}


auto AssetImporter::import_texture(Path src_filepath, ImportTextureParams params)
    -> Job<UUID>
{
    return detail::import_texture_async({ *this }, MOVE(src_filepath), params);
}


auto AssetImporter::import_model(Path path, ImportModelParams params)
    -> Job<UUID>
{
    return detail::import_model_async({ *this }, MOVE(path), params);
}


} // namespace josh
