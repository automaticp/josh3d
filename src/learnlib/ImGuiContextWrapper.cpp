#include "ImGuiContextWrapper.hpp"
#include <glfwpp/window.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace learn {

ImGuiContextWrapper::ImGuiContextWrapper(glfw::Window& window) {
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    auto [x, y] = window.getContentScale();
    ImGui::GetStyle().ScaleAllSizes(x);
    ImGui::GetIO().FontGlobalScale = x;
    ImGui::GetIO().IniFilename = nullptr;
}

void ImGuiContextWrapper::new_frame() const {
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiContextWrapper::render() const {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

ImGuiContextWrapper::~ImGuiContextWrapper() noexcept {
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}


bool ImGuiInputBlocker::is_key_blocked(const KeyCallbackArgs&) const noexcept {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGuiInputBlocker::is_cursor_blocked(const CursorPosCallbackArgs&) const noexcept {
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiInputBlocker::is_scroll_blocked(const ScrollCallbackArgs&) const noexcept {
    return ImGui::GetIO().WantCaptureMouse;
}


} // namespace learn
