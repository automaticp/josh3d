#include "ResourceLoader.hpp"
#include "ECS.hpp"
#include "detail/ResourceLoader.hpp"


namespace josh {


ResourceLoader::ResourceLoader(
    Registry&          scene_registry,
    ResourceDatabase&  resource_database,
    ResourceRegistry&  resource_registry,
    ThreadPool&        loading_pool,
    OffscreenContext&  offscreen_context,
    CompletionContext& completion_context
)
    : scene_registry_    { scene_registry     }
    , resource_database_ { resource_database  }
    , resource_registry_ { resource_registry  }
    , thread_pool_       { loading_pool       }
    , offscreen_context_ { offscreen_context  }
    , completion_context_{ completion_context }
{}


auto ResourceLoader::load_scene(const UUID& uuid, Handle dst_handle)
    -> Job<void>
{
    return detail::load_scene_async({ *this }, uuid, dst_handle);
}




} // namespace josh
