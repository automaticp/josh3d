#pragma once
#include "EnumUtils.hpp"
#include "ImGuiHelpers.hpp"
#include <imgui.h>


/*
Various extensions to ImGui.

Not sure if merging namespaces is a good idea in the long run,
so all imgui calls are qualified in case we'll change the namespace later.
*/
namespace ImGui {

/*
Wrapper of ImGui::Image that flips the image UVs
to accomodate the OpenGL bottom-left origin.
*/
inline void ImageGL(
    ImTextureID   image_id,
    const ImVec2& size,
    const ImVec4& tint_color = { 1.0f, 1.0f, 1.0f, 1.0f },
    const ImVec4& border_color = { 1.0f, 1.0f, 1.0f, 1.0f }) noexcept
{
    ImGui::Image(image_id, size, ImVec2{ 0.f, 1.f }, ImVec2{ 1.f, 0.f }, tint_color, border_color);
}

inline void ImageGL(
    unsigned int  image_id,
    const ImVec2& size,
    const ImVec4& tint_color = { 1.0f, 1.0f, 1.0f, 1.0f },
    const ImVec4& border_color = { 1.0f, 1.0f, 1.0f, 1.0f }) noexcept
{
    ImageGL(josh::void_id(image_id), size, tint_color, border_color);
}

inline void TextUnformatted(std::string_view str)
{
    ImGui::TextUnformatted(str.begin(), str.end());
}

/*
Combo box for enums with "extras".
*/
template<josh::enumeration E>
auto EnumCombo(
    const char*          label,
    E*                   enumerant,
    ImGuiComboFlags      combo_flags = {},
    ImGuiSelectableFlags selectable_flags = {})
    -> bool
{
    if (ImGui::BeginCombo(label, enum_cstring(*enumerant), combo_flags))
    {
        for (const auto e : enum_iter(E()))
            if (ImGui::Selectable(enum_cstring(e), e == *enumerant, selectable_flags))
                *enumerant = e;
        ImGui::EndCombo();
        return true;
    }
    return false;
}

} // namespace ImGui
