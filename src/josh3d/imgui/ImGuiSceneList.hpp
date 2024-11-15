#pragma once
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
        AssetUnpacker&  asset_unpacker,
        SceneImporter&  scene_importer
    )
        : registry_      { registry       }
        , asset_unpacker_{ asset_unpacker }
        , scene_importer_{ scene_importer }
    {}

    void display();

private:
    entt::registry& registry_;
    AssetUnpacker&  asset_unpacker_;
    SceneImporter&  scene_importer_;
};


} // namespace josh
