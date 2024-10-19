#pragma once
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
        SceneImporter&  importer
    )
        : registry_{ registry }
        , importer_{ importer }
    {}

    void display();

private:
    entt::registry& registry_;
    SceneImporter&  importer_;
};


} // namespace josh
