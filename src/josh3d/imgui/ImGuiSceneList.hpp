#pragma once
#include "AssetManager.hpp"
#include "AssetUnpacker.hpp"
#include "SceneImporter.hpp"
#include <entt/entity/fwd.hpp>


namespace josh {


/*
List of scene entities with proper scene-graph nesting.
*/
class ImGuiSceneList {
public:
    ImGuiSceneList(
        entt::registry& registry,
        AssetManager&   asset_manager,
        AssetUnpacker&  asset_unpacker,
        SceneImporter&  scene_importer
    )
        : registry_      { registry       }
        , asset_manager_ { asset_manager  }
        , asset_unpacker_{ asset_unpacker }
        , scene_importer_{ scene_importer }
    {}

    void display();

private:
    entt::registry& registry_;
    AssetManager&   asset_manager_;
    AssetUnpacker&  asset_unpacker_;
    SceneImporter&  scene_importer_;
};


} // namespace josh
