#include "DefaultImporters.hpp"
#include "detail/AssimpCommon.hpp"


namespace josh {


auto import_scene(
    AssetImporterContext context,
    Path                 path,
    ImportSceneParams    params)
        -> Job<UUID>
{
    return detail::import_scene_async(MOVE(context), MOVE(path), MOVE(params));
}


} // namespace josh
