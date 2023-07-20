#pragma once
#include <concepts>
#include <imgui.h>
#include <type_traits>


namespace josh {


inline void* void_id(std::integral auto value) noexcept {
    return reinterpret_cast<void*>(value);
}

template<typename EnumT>
    requires std::is_enum_v<EnumT>
inline void* void_id(EnumT value) noexcept {
    return void_id(static_cast<std::underlying_type_t<EnumT>>(value));
}


} // namespace josh


namespace ImGui {


// Wrapper of ImGui::Image that flips the image UVs
// to accomodate the OpenGL bottom-left origin.
inline void ImageGL(ImTextureID image_id,
    const ImVec2& size,
    const ImVec4& tint_color = { 1.0f, 1.0f, 1.0f, 1.0f },
    const ImVec4& border_color = { 1.0f, 1.0f, 1.0f, 1.0f }) noexcept
{
    Image(
        image_id, size, ImVec2{ 0.f, 1.f }, ImVec2{ 1.f, 0.f },
        tint_color, border_color
    );
}


} // namespace ImGui
