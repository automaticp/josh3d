#pragma once
#include "ContainerUtils.hpp"
#include "EnumUtils.hpp"
#include "ImGuiHelpers.hpp"
#include <cinttypes>
#include <cstdint>
#include <imgui.h>
#include <imgui_internal.h>
#include <type_traits>


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

template<josh::enumeration E>
auto EnumListBox(
    const char*          label,
    E*                   enumerant,
    const ImVec2&        size,
    ImGuiSelectableFlags selectable_flags = {})
        -> bool
{
    if (ImGui::BeginListBox(label, size))
    {
        for (const auto e : enum_iter(E()))
            if (ImGui::Selectable(enum_cstring(e), e == *enumerant, selectable_flags))
                *enumerant = e;
        ImGui::EndListBox();
        return true;
    }
    return false;
}

/*
If `height_in_items` is 0, then it is taken as `enum_size<E>()`.
*/
template<josh::enumeration E>
auto EnumListBox(
    const char*          label,
    E*                   enumerant,
    int                  height_in_items  = 0,
    ImGuiSelectableFlags selectable_flags = {})
        -> bool
{
    if (height_in_items == 0)
        height_in_items = enum_size(E());

    const ImVec2 wh = {
        0,
        // This is *almost* exact. Almost...
        float(height_in_items) * ImGui::GetFrameHeight()
    };

    return EnumListBox(label, enumerant, wh, selectable_flags);
}

namespace detail {
template<typename T> struct datatype;
template<typename T> struct default_fmt;
template<typename T> constexpr auto datatype_v = datatype<T>::value;
template<typename T> constexpr auto default_fmt_v = default_fmt<T>::value;
#define JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(Type, ImType, Fmt) \
    template<> struct datatype<Type> : josh::value_constant<ImGuiDataType_##ImType> {}; \
    template<> struct default_fmt<Type> { static constexpr auto value = Fmt; }
JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(uint8_t,  U8,  "%" PRIu8);
JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(uint16_t, U16, "%" PRIu16);
JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(uint32_t, U32, "%" PRIu32);
JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(uint64_t, U64, "%" PRIu64);
JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(int8_t,   S8,  "%" PRIi8);
JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(int16_t,  S16, "%" PRIi16);
JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(int32_t,  S32, "%" PRIi32);
JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(int64_t,  S64, "%" PRIi64);
JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(float,    Float,  "%.3f");
JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS(double,   Double, "%.3f");
#undef JOSH3D_IMGUI_DEFINE_DATATYPE_TRAITS
} // namespace detail

template<typename T> using not_deduced = std::type_identity_t<T>;

template<typename T>
auto SliderScalar(
    const char*      label,
    T*               v,
    not_deduced<T>   min,
    not_deduced<T>   max,
    const char*      format = detail::default_fmt_v<T>,
    ImGuiSliderFlags flags  = {})
        -> bool
{
    return ImGui::SliderScalar(label, detail::datatype_v<T>,
        v, &min, &max, format, flags);
}

template<typename T>
auto DragScalar(
    const char*      label,
    T*               v,
    not_deduced<T>   min     = {},
    not_deduced<T>   max     = {},
    float            v_speed = 1.f,
    const char*      format  = detail::default_fmt_v<T>,
    ImGuiSliderFlags flags   = {})
        -> bool
{
    return ImGui::DragScalar(label, detail::datatype_v<T>,
        v, v_speed, &min, &max, format, flags);
}

} // namespace ImGui
