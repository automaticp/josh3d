#pragma once
#include <entt/entity/fwd.hpp>


namespace josh {


/*
List of scene entities with proper scene-graph nesting.
*/
class ImGuiSceneList {
public:
    ImGuiSceneList(entt::registry& registry) : registry_{ registry } {}

    void display();

private:
    entt::registry& registry_;
};


} // namespace josh
