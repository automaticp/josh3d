#include "AssetImporter.hpp"
#include "AsyncCradle.hpp"
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
    AsyncCradleRef     async_cradle
)
    : resource_database_(resource_database)
    , cradle_           (async_cradle)
{}


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
