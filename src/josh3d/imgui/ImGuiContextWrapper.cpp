#include "ImGuiContextWrapper.hpp"
#include <glfwpp/window.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace josh {

ImGuiContextWrapper::ImGuiContextWrapper(glfw::Window& window)
{
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    auto [x, y] = window.getContentScale();
    ImGui::GetStyle().ScaleAllSizes(x);
    ImGui::GetIO().FontGlobalScale = x;
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void ImGuiContextWrapper::new_frame() const
{
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiContextWrapper::render() const
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

ImGuiContextWrapper::~ImGuiContextWrapper() noexcept
{
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}


} // namespace josh
