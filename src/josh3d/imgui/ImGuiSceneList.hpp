#pragma once
#include "AssetManager.hpp"
#include "AssetUnpacker.hpp"
#include "SceneImporter.hpp"
#include "ECS.hpp"


namespace josh {


/*
List of scene entities with proper scene-graph nesting.
*/
struct ImGuiSceneList
{
    Registry&      registry;
    AssetManager&  asset_manager;
    AssetUnpacker& asset_unpacker;
    SceneImporter& scene_importer;

    void display();
};


} // namespace josh
