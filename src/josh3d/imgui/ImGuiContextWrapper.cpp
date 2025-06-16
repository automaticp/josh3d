#include "ImGuiContextWrapper.hpp"
#include "Fonts.hpp" // IWYU pragma: keep (are you blind?)
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
    ImGui::GetStyle().FontScaleDpi = x;
    ImGui::GetStyle().ScaleAllSizes(x);
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;

    ImFontConfig font_cfg = {};
    font_cfg.FontDataOwnedByAtlas = false;
    font_cfg.SizePixels = 14; // TODO: Should be configurable.
    using namespace fonts;

#define ADD_FONT(FName) \
    std::strncpy(font_cfg.Name, #FName, std::size(font_cfg.Name) - 1); \
    io.Fonts->AddFontFromMemoryCompressedTTF(FName##_compressed_data, int(FName##_compressed_size), 0.f, &font_cfg)
    ADD_FONT(RobotoMedium); // Default. TODO: Should be configurable.
    ADD_FONT(CousineRegular);
    ADD_FONT(KarlaRegular);
    ADD_FONT(DroidSans);
    ADD_FONT(ProggyClean);
    ADD_FONT(ProggyTiny);
#undef ADD_FONT
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
