#pragma once
#include "UIContextFwd.hpp"


namespace josh {

/*
FIXME: This is really not the place to *cache* the window state.
This is not the window "controller". It's just some lonely widget.

Either ApplicationAssembly has to do this, or something "above" it.
Maybe some window wrapper?
*/
struct ImGuiWindowSettings
{
    void display(UIContext& ui);

    bool _is_vsync_on = false; // FIXME: assumed, not guaranteed

    struct WindowedParams
    {
        int xpos, ypos, width, height;
    };

    WindowedParams _last_params = {}; // Saved before going fullscreen.
};

} // namespace josh
