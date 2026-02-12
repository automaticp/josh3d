#pragma once
#include "UIContextFwd.hpp"


namespace josh {

/*
List of scene entities with proper scene-graph nesting.
*/
struct ImGuiSceneList
{
    void display(UIContext& ui);
};

} // namespace josh
