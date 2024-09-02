#pragma once
#include "DefaultTextures.hpp"
#include "GLObjects.hpp"
#include "ImGuiHelpers.hpp"
#include "LightCasters.hpp"
#include "Model.hpp"
#include "Materials.hpp"
#include "Name.hpp"
#include "Tags.hpp"
#include "Filesystem.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "tags/AlphaTested.hpp"
#include "tags/Culled.hpp"
#include "tags/Selected.hpp"
#include "tags/ShadowCasting.hpp"
#include <cassert>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glm/fwd.hpp>
#include <imgui.h>


namespace josh::imgui {


inline bool TransformWidget(josh::Transform* transform) noexcept {

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


inline void Matrix4x4DisplayWidget(const glm::mat4& mat4) {
    const size_t num_rows = 4;
    const size_t num_cols = 4;
    ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_NoHostExtendX;
    if (ImGui::BeginTable("Matrix4x4", num_cols, flags)) {
        for (size_t row = 0; row < num_rows; ++row) {
            ImGui::TableNextRow();
            for (size_t col = 0; col < num_cols; ++col) {
                ImGui::TableSetColumnIndex(int(col));

                ImGui::Text("%.3f", mat4[col][row]);
            }
        }
        ImGui::EndTable();
    }
}


inline void Matrix3x3DisplayWidget(const glm::mat3& mat3) {
    const size_t num_rows = 3;
    const size_t num_cols = 3;
    ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_NoHostExtendX;
    if (ImGui::BeginTable("Matrix4x4", num_cols, flags)) {
        for (size_t row = 0; row < num_rows; ++row) {
            ImGui::TableNextRow();
            for (size_t col = 0; col < num_cols; ++col) {
                ImGui::TableSetColumnIndex(int(col));

                ImGui::Text("%.3f", mat3[col][row]);
            }
        }
        ImGui::EndTable();
    }
}


inline void NameWidget(josh::Name* name) noexcept {
    ImGui::Text("Name: %s", name->c_str());
}


inline void PathWidget(josh::Path* path) noexcept {
    ImGui::Text("Path: %s", path->c_str());
}


inline void VPathWidget(josh::VPath* vpath) noexcept {
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
    if (auto material = mesh.try_get<MaterialDiffuse>()) {
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


    if (auto material = mesh.try_get<MaterialSpecular>()) {
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


    if (auto material = mesh.try_get<MaterialNormal>()) {
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
        bool is_alpha_tested = mesh.all_of<AlphaTested>();
        ImGui::SameLine();
        if (ImGui::Checkbox("Alpha-Testing", &is_alpha_tested)) {
            if (is_alpha_tested) {
                mesh.emplace<AlphaTested>();
            } else {
                mesh.remove<AlphaTested>();
            }
        }
    }
}


inline bool SelectButton(entt::handle handle) noexcept {
    bool feedback{ false };

    const bool is_selected = handle.all_of<Selected>();
    constexpr ImVec4 selected_color{ .2f, .6f, .2f, 1.f };

    ImGui::PushID(void_id(handle.entity()));

    if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, selected_color);

        if (ImGui::SmallButton("Select")) {
            handle.erase<Selected>();
            feedback = true;
        }

        ImGui::PopStyleColor();
    } else /* not selected */ {
        if (ImGui::SmallButton("Select")) {
            handle.emplace<Selected>();
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

    const bool is_culled = mesh_handle.all_of<Culled>();
    if (is_culled) {
        auto text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        text_color.w *= 0.5f; // Dim text when culled.
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    }

    if (auto name = mesh_handle.try_get<Name>()) {
        ImGui::Text("Mesh [%d]: %s", entt::to_entity(mesh_handle.entity()), name->c_str());
    } else {
        ImGui::Text("Mesh [%d]",     entt::to_entity(mesh_handle.entity()));
    }

    if (is_culled) {
        ImGui::PopStyleColor();
    }

}


inline void MeshWidgetBody(entt::handle mesh_handle) noexcept {

    if (auto tf = mesh_handle.try_get<Transform>()) {
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

    if (auto name = model_handle.try_get<Name>()) {
        ImGui::Text("Model [%d]: %s", entt::to_entity(model_handle.entity()), name->c_str());
    } else {
        ImGui::Text("Model [%d]",     entt::to_entity(model_handle.entity()));
    }

    return feedback;
}


inline void ModelWidgetBody(entt::handle model_handle) noexcept {

    if (auto tf = model_handle.try_get<Transform>()) {
        TransformWidget(tf);
    }

    if (auto path = model_handle.try_get<Path>()) {
        PathWidget(path);
    }

    if (auto vpath = model_handle.try_get<VPath>()) {
        VPathWidget(vpath);
    }


    if (ImGui::TreeNode("Meshes")) {

        auto model = model_handle.try_get<Model>();
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


inline bool AmbientLightWidget(AmbientLight* alight) {
    return ImGui::ColorEdit3("Color", glm::value_ptr(alight->color), ImGuiColorEditFlags_DisplayHSV);
}


inline bool DirectionalLightWidget(entt::handle dlight_handle) {
    bool feedback = false;

    if (auto* dlight = dlight_handle.try_get<DirectionalLight>()) {

        feedback |= ImGui::ColorEdit3("Color", glm::value_ptr(dlight->color), ImGuiColorEditFlags_DisplayHSV);
        ImGui::SameLine();
        bool has_shadow = has_tag<ShadowCasting>(dlight_handle);
        if (ImGui::Checkbox("Shadow", &has_shadow)) {
            switch_tag<ShadowCasting>(dlight_handle);
            feedback |= true;
        }


        // TODO: Might actually make sense to represent direction
        // as theta and phi pair internally. That way, there's no degeneracy.

        // We swap x and y so that phi is rotation around x,
        // and behaves more like the real Sun.
        // We're probably not on the north pole, it's fine.
        const glm::vec3 dir = dlight->direction;
        glm::vec3 swapped_dir{ dir.y, dir.x, dir.z };
        glm::vec2 polar = glm::degrees(glm::polar(swapped_dir));
        if (ImGui::DragFloat2("Direction", glm::value_ptr(polar), 0.5f)) {
            swapped_dir = glm::euclidean(glm::radians(glm::vec2{ polar.x, polar.y }));
            // Un-swap back.
            dlight->direction = glm::vec3{ swapped_dir.y, swapped_dir.x, swapped_dir.z };
            feedback |= true;
        }

        ImGui::BeginDisabled();
        ImGui::InputFloat3("Direction XYZ", glm::value_ptr(dlight->direction));
        ImGui::EndDisabled();

    }

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

    if (auto plight = plight_handle.try_get<PointLight>()) {

        ImGui::DragFloat3("Position", glm::value_ptr(plight->position), 0.2f);
        ImGui::ColorEdit3("Color", glm::value_ptr(plight->color), ImGuiColorEditFlags_DisplayHSV);
        ImGui::SameLine();
        bool has_shadow = has_tag<ShadowCasting>(plight_handle);
        if (ImGui::Checkbox("Shadow", &has_shadow)) {
            switch_tag<ShadowCasting>(plight_handle);
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
