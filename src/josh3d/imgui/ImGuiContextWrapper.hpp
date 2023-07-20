#pragma once


namespace glfw { class Window; }

namespace josh {


class ImGuiContextWrapper {
public:
    ImGuiContextWrapper(glfw::Window& window);

    void new_frame() const;

    void render() const;

    ImGuiContextWrapper(const ImGuiContextWrapper&) = delete;
    ImGuiContextWrapper& operator=(const ImGuiContextWrapper&) = delete;

    ~ImGuiContextWrapper() noexcept;
};


} // namespace josh
