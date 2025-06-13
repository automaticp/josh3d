#pragma once
#include "ECS.hpp"


namespace josh {


struct ImGuiSelected
{
    Registry& registry;

    bool display_model_matrix   = false;
    bool display_all_components = false;

    void display();
};


} // namespace josh
