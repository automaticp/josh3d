#pragma once
#include "UIContextFwd.hpp"


namespace josh {


struct ImGuiSelected
{
    bool display_model_matrix   = false;
    bool display_all_components = false;

    void display(UIContext& ui);
};


} // namespace josh
