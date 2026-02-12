#pragma once
#include "Semantics.hpp"


namespace glfw { class Window; }

namespace josh {

struct ImGuiContextWrapper
    : Immovable<ImGuiContextWrapper>
{
    ImGuiContextWrapper(glfw::Window& window);

    void new_frame() const;
    void render() const;

    ~ImGuiContextWrapper() noexcept;
};

} // namespace josh
