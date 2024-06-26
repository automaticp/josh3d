#pragma once
#include "DefaultTextures.hpp"
#include "ECSHelpers.hpp"
#include "GLObjects.hpp"
#include "ImGuiHelpers.hpp"
#include "LightCasters.hpp"
#include "components/Model.hpp"
#include "components/Materials.hpp"
#include "components/Name.hpp"
#include "components/Path.hpp"
#include "components/Transform.hpp"
#include "components/VPath.hpp"
#include "tags/AlphaTested.hpp"
#include "tags/Culled.hpp"
#include "tags/Selected.hpp"
#include "tags/ShadowCasting.hpp"
#include <cassert>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <imgui.h>


namespace josh::imgui {


inline bool TransformWidget(josh::components::Transform* transform) noexcept {

    bool feedback{ false };

    feedback |= ImGui::DragFloat3(
        "Position", glm::value_ptr(transform->position()),
        0.2f, -FLT_MAX, FLT_MAX
    );

    // FIXME: This is slightly more usable, but the singularity for Pitch around 90d
    // is still unstable. In general: Local X is Pitch, Global Y is Yaw, and Local Z is Roll.
    // Stll very messy to use, but should get the ball rolling.
    const glm::quat& q = transform->rotation();
    // Swap quaternion axes to make pitch around (local) X axis.
    // Also GLM for some reason assumes that the locking [-90, 90] axis is
    // associated with Yaw, not Pitch...? Maybe I am confused here but
    // I want it Pitch, so we also have to swap the euler representation.
    // (In my mind, Pitch and Yaw are Theta and Phi in spherical coordinates respectively).
    const glm::quat q_shfl = glm::quat{ q.w, q.y, q.x, q.z };
    glm::vec3 euler = glm::degrees(glm::vec3{
        glm::yaw(q_shfl),   // Pitch
        glm::pitch(q_shfl), // Yaw
        glm::roll(q_shfl)   // Roll
        // Dont believe what GLM says
    });
    if (ImGui::DragFloat3("Pitch/Yaw/Roll", glm::value_ptr(euler), 1.0f, -360.f, 360.f, "%.3f")) {
        euler.x = glm::clamp(euler.x, -89.999f, 89.999f);
        euler.y = glm::mod(euler.y, 360.f);
        euler.z = glm::mod(euler.z, 360.f);
        // Un-shuffle back both the euler angles and quaternions.
        glm::quat p = glm::quat(glm::radians(glm::vec3{ euler.y, euler.x, euler.z }));
        transform->rotation() = glm::quat{ p.w, p.y, p.x, p.z };
        feedback |= true;
    }

    feedback |= ImGui::DragFloat3(
        "Scale", glm::value_ptr(transform->scaling()),
        0.1f, 0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    return feedback;
}


inline void NameWidget(josh::components::Name* name) noexcept {
    ImGui::Text("Name: %s", name->name.c_str());
}


inline void PathWidget(josh::components::Path* path) noexcept {
    ImGui::Text("Path: %s", path->c_str());
}


inline void VPathWidget(josh::components::VPath* vpath) noexcept {
    ImGui::Text("VPath: %s", vpath->path().c_str());
}


inline void MaterialsWidget(entt::handle mesh) noexcept {

    const float  text_height = ImGui::GetTextLineHeight();
    const ImVec2 preview_size    { 4.f * text_height, 4.f * text_height };

    constexpr ImVec4 tex_tint        { 1.f, 1.f, 1.f, 1.f };
    constexpr ImVec4 tex_frame_color { .5f, .5f, .5f, .5f };
    constexpr ImVec4 empty_slot_color{ .8f, .8f, .2f, .8f };
    constexpr ImVec4 empty_slot_tint { 1.f, 1.f, 1.f, .2f };

    const auto material_widget = [&](
        RawTexture2D<GLConst> texture,
        const char*           name,
        const char*           popup_str_id,
        const ImVec4&         frame_color,
        const ImVec4&         tint_color)
    {
        const auto tex_id = void_id(texture.id());
        imgui::ImageGL(tex_id, preview_size, tint_color, frame_color);
        // All hover and click tests below are for this image above ^.

        const auto hover_widget = [&] {
            const Size2I resolution = texture.get_resolution();
            const float aspect = resolution.aspect_ratio();

            const float largest_side = 512.f; // Desired.

            const ImVec2 hovered_size = [&]() -> ImVec2 {
                const auto [w, h] = resolution;
                if (w == h) {
                    return { largest_side,          largest_side          };
                } else if (w < h) {
                    return { largest_side * aspect, largest_side          };
                } else /* if (w > h) */ {
                    return { largest_side,          largest_side / aspect };
                }
            }();

            imgui::ImageGL(tex_id, hovered_size, tex_tint, frame_color);
            ImGui::Text("%s, %dx%d", name, resolution.width, resolution.height);
        };



        auto bg_col = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
        bg_col.w = .6f; // Make less opaque
        auto border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
        border_col.x *= 1.6f; // Lighten
        border_col.y *= 1.6f;
        border_col.z *= 1.6f;

        ImGui::PushStyleColor(ImGuiCol_PopupBg, bg_col);
        ImGui::PushStyleColor(ImGuiCol_Border,  border_col);


        // We want the popup to appear seamlessly, replacing the tooltip,
        // so we store the position of the tooltip window.
        ImVec2 tooltip_pos{};

        if (ImGui::IsItemHovered()) {

            ImGui::BeginTooltip();
            tooltip_pos = ImGui::GetWindowPos();
            hover_widget();

            ImGui::EndTooltip();
        }


        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            ImGui::SetNextWindowPos(tooltip_pos);
            ImGui::OpenPopup(popup_str_id);
        }

        if (ImGui::BeginPopup(popup_str_id)) {

            hover_widget();

            ImGui::EndPopup();
        }


        ImGui::PopStyleColor(2);


    };


    bool can_be_alpha_tested{ false };
    if (auto material = mesh.try_get<components::MaterialDiffuse>()) {
        material_widget(
            material->texture, "Diffuse", "Diffuse Popup", tex_frame_color, tex_tint
        );

        can_be_alpha_tested =
            material->texture->get_component_type<PixelComponent::Alpha>() != PixelComponentType::None;

    } else {
        material_widget(
            globals::default_diffuse_texture(), "Diffuse (Default)", "Diffuse Popup", empty_slot_color, empty_slot_tint
        );
    }


    if (auto material = mesh.try_get<components::MaterialSpecular>()) {
        ImGui::SameLine();
        material_widget(
            material->texture, "Specular", "Specular Popup", tex_frame_color, tex_tint
        );
    } else {
        ImGui::SameLine();
        material_widget(
            globals::default_specular_texture(), "Specular (Default)", "Specular Popup", empty_slot_color, empty_slot_tint
        );
    }


    if (auto material = mesh.try_get<components::MaterialNormal>()) {
        ImGui::SameLine();
        material_widget(
            material->texture, "Normal", "Normal Popup", tex_frame_color, tex_tint
        );
    } else {
        ImGui::SameLine();
        material_widget(
            globals::default_normal_texture(), "Normal (Default)", "Normal Popup", empty_slot_color, empty_slot_tint
        );
    }


    if (can_be_alpha_tested) {
        bool is_alpha_tested = mesh.all_of<tags::AlphaTested>();
        ImGui::SameLine();
        if (ImGui::Checkbox("Alpha-Testing", &is_alpha_tested)) {
            if (is_alpha_tested) {
                mesh.emplace<tags::AlphaTested>();
            } else {
                mesh.remove<tags::AlphaTested>();
            }
        }
    }
}


inline bool SelectButton(entt::handle handle) noexcept {
    bool feedback{ false };

    const bool is_selected = handle.all_of<tags::Selected>();
    constexpr ImVec4 selected_color{ .2f, .6f, .2f, 1.f };

    ImGui::PushID(void_id(handle.entity()));

    if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, selected_color);

        if (ImGui::SmallButton("Select")) {
            handle.erase<tags::Selected>();
            feedback = true;
        }

        ImGui::PopStyleColor();
    } else /* not selected */ {
        if (ImGui::SmallButton("Select")) {
            handle.emplace<tags::Selected>();
            feedback = true;
        }
    }

    ImGui::PopID();

    return feedback;
}


// Does not remove anything, just signals that it has been pressed.
[[nodiscard]]
inline bool RemoveButton() noexcept {
    bool feedback{ false };

    if (ImGui::SmallButton("Remove")) {
        feedback = true;
    }

    return feedback;
}


inline void MeshWidgetHeader(entt::handle mesh_handle) noexcept {

    SelectButton(mesh_handle);
    ImGui::SameLine();

    const bool is_culled = mesh_handle.all_of<tags::Culled>();
    if (is_culled) {
        auto text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        text_color.w *= 0.5f; // Dim text when culled.
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    }

    if (auto name = mesh_handle.try_get<components::Name>()) {
        ImGui::Text("Mesh [%d]: %s", entt::to_entity(mesh_handle.entity()), name->name.c_str());
    } else {
        ImGui::Text("Mesh [%d]",     entt::to_entity(mesh_handle.entity()));
    }

    if (is_culled) {
        ImGui::PopStyleColor();
    }

}


inline void MeshWidgetBody(entt::handle mesh_handle) noexcept {

    if (auto tf = mesh_handle.try_get<components::Transform>()) {
        TransformWidget(tf);
    }

    MaterialsWidget(mesh_handle);
}


inline void MeshWidget(entt::handle mesh_handle) noexcept {

    ImGui::PushID(void_id(mesh_handle.entity()));

    MeshWidgetHeader(mesh_handle);
    MeshWidgetBody(mesh_handle);

    ImGui::PopID();
}


enum class Feedback {
    None,
    Remove
};


[[nodiscard]]
inline Feedback ModelWidgetHeader(entt::handle model_handle) noexcept {
    Feedback feedback{ Feedback::None };

    // ImGui::PushID(void_id(model_handle.entity()));

    SelectButton(model_handle);
    ImGui::SameLine();
    if (RemoveButton()) { feedback = Feedback::Remove; }
    ImGui::SameLine();

    if (auto name = model_handle.try_get<components::Name>()) {
        ImGui::Text("Model [%d]: %s", entt::to_entity(model_handle.entity()), name->name.c_str());
    } else {
        ImGui::Text("Model [%d]",     entt::to_entity(model_handle.entity()));
    }

    return feedback;
}


inline void ModelWidgetBody(entt::handle model_handle) noexcept {

    if (auto tf = model_handle.try_get<components::Transform>()) {
        TransformWidget(tf);
    }

    if (auto path = model_handle.try_get<components::Path>()) {
        PathWidget(path);
    }

    if (auto vpath = model_handle.try_get<components::VPath>()) {
        VPathWidget(vpath);
    }


    if (ImGui::TreeNode("Meshes")) {

        auto model = model_handle.try_get<components::Model>();
        assert(model);

        for (auto mesh_entity : model->meshes()) {    ;
            MeshWidget({ *model_handle.registry(), mesh_entity });
        }

        ImGui::TreePop();
    }

}


[[nodiscard]]
inline Feedback ModelWidget(entt::handle model_handle) noexcept {

    ImGui::PushID(void_id(model_handle.entity()));

    auto feedback = ModelWidgetHeader(model_handle);
    ModelWidgetBody(model_handle);

    ImGui::PopID();

    return feedback;
}


inline Feedback PointLightWidgetHeader(entt::handle plight_handle) noexcept {
    Feedback feedback{ Feedback::None };

    SelectButton(plight_handle);
    ImGui::SameLine();
    if (RemoveButton()) { feedback = Feedback::Remove; }
    ImGui::SameLine();

    ImGui::Text("Point Light [%d]", entt::to_entity(plight_handle.entity()));

    return feedback;
}


inline void PointLightWidgetBody(entt::handle plight_handle) noexcept {

    if (auto plight = plight_handle.try_get<light::Point>()) {

        ImGui::DragFloat3("Position", glm::value_ptr(plight->position), 0.2f);
        ImGui::ColorEdit3("Color", glm::value_ptr(plight->color), ImGuiColorEditFlags_DisplayHSV);
        ImGui::SameLine();
        bool has_shadow = has_tag<tags::ShadowCasting>(plight_handle);
        if (ImGui::Checkbox("Shadow", &has_shadow)) {
            switch_tag<tags::ShadowCasting>(plight_handle);
        }

        ImGui::DragFloat3(
            "Atten. (c/l/q)", &plight->attenuation.constant,
            0.1f, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic
        );

    }
}


inline Feedback PointLightWidget(entt::handle plight_handle) noexcept {
    ImGui::PushID(void_id(plight_handle.entity()));

    Feedback feedback = PointLightWidgetHeader(plight_handle);
    PointLightWidgetBody(plight_handle);

    ImGui::PopID();
    return feedback;
}




} // namespace josh::imgui
