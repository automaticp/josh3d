#pragma once
#include "AssetImporter.hpp"
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
        AssetImporter&  asset_importer,
        SceneImporter&  scene_importer
    )
        : registry_      { registry       }
        , asset_importer_{ asset_importer }
        , scene_importer_{ scene_importer }
    {}

    void display();

private:
    entt::registry& registry_;
    AssetImporter&  asset_importer_;
    SceneImporter&  scene_importer_;
};


} // namespace josh
