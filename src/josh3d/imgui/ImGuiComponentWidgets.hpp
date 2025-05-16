#pragma once
#include "AABB.hpp"
#include "Active.hpp"
#include "BoundingSphere.hpp"
#include "Camera.hpp"
#include "Components.hpp"
#include "DefaultTextures.hpp"
#include "ECS.hpp"
#include "GLObjects.hpp"
#include "ImGuiHelpers.hpp"
#include "LightCasters.hpp"
#include "Mesh.hpp"
#include "Materials.hpp"
#include "Name.hpp"
#include "SceneGraph.hpp"
#include "SkeletalAnimation.hpp"
#include "SkinnedMesh.hpp"
#include "Skybox.hpp"
#include "Tags.hpp"
#include "Filesystem.hpp"
#include "TerrainChunk.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "tags/AlphaTested.hpp"
#include "tags/ShadowCasting.hpp"
#include "tags/Visible.hpp"
#include <cassert>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>


namespace josh::imgui {


struct GenericHeaderInfo {
    const char* type_name = "UnknownEntityType";
    const char* name      = "";
};


inline auto GetGenericHeaderInfo(entt::const_handle handle)
    -> GenericHeaderInfo
{
    const char* type_name = [&]() {
        if (has_component<Mesh>            (handle)) { return "Mesh";             }
        if (has_component<SkinnedMesh>     (handle)) { return "SkinnedMesh";      }
        if (has_component<TerrainChunk>    (handle)) { return "TerrainChunk";     }
        if (has_component<AmbientLight>    (handle)) { return "AmbientLight";     }
        if (has_component<DirectionalLight>(handle)) { return "DirectionalLight"; }
        if (has_component<PointLight>      (handle)) { return "PointLight";       }
        if (has_component<Skybox>          (handle)) { return "Skybox";           }
        if (has_component<Camera>          (handle)) { return "Camera";           }

        if (has_component<Transform>(handle)) {
            if (has_children(handle)) { return "Node";   }
            else                      { return "Orphan"; }
        } else {
            if (has_children(handle)) { return "GroupingNode";  } // Does this even make sense?
            else                      { return "UnknownEntity"; }
        }
    }();

    const char* name = [&]() {
        if (const Name* name = handle.try_get<Name>()) {
            return name->c_str();
        } else {
            return "";
        }
    }();

    return { type_name, name };
}


inline bool GetGenericVisibility(entt::const_handle handle) {
    if (has_component<AmbientLight>    (handle)) { return is_active<AmbientLight>    (handle); }
    if (has_component<DirectionalLight>(handle)) { return is_active<DirectionalLight>(handle); }
    if (has_component<Skybox>          (handle)) { return is_active<Skybox>          (handle); }
    if (has_component<Camera>          (handle)) { return is_active<Camera>          (handle); }

    if (handle.any_of<AABB, BoundingSphere>()) {
        return has_tag<Visible>(handle);
    } else {
        return true;
    }
}


inline void GenericHeaderText(entt::const_handle handle) {
    const bool is_visible = GetGenericVisibility(handle);
    if (!is_visible) {
        auto text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        text_color.w *= 0.5f; // Dim text when culled.
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    }

    auto [type_name, name] = GetGenericHeaderInfo(handle);
    ImGui::Text("[%d] [%s] %s", entt::to_entity(handle.entity()), type_name, name);

    if (!is_visible) {
        ImGui::PopStyleColor();
    }
}


struct GenericActiveInfo {
    bool can_be_active{ false };
    bool is_active    { false };
};


inline auto GetGenericActiveInfo(entt::const_handle handle)
    -> GenericActiveInfo
{
    if (has_component<AmbientLight>    (handle)) { return { true, is_active<AmbientLight>    (handle) }; }
    if (has_component<DirectionalLight>(handle)) { return { true, is_active<DirectionalLight>(handle) }; }
    if (has_component<Skybox>          (handle)) { return { true, is_active<Skybox>          (handle) }; }
    if (has_component<Camera>          (handle)) { return { true, is_active<Camera>          (handle) }; }
    return { false, false };
}


inline void GenericMakeActive(entt::handle handle) {
    if (has_component<AmbientLight>    (handle)) { make_active<AmbientLight>    (handle); }
    if (has_component<DirectionalLight>(handle)) { make_active<DirectionalLight>(handle); }
    if (has_component<Skybox>          (handle)) { make_active<Skybox>          (handle); }
    if (has_component<Camera>          (handle)) { make_active<Camera>          (handle); }
}



inline bool TransformWidget(josh::Transform& transform) noexcept {

    bool feedback{ false };

    feedback |= ImGui::DragFloat3(
        "Position", glm::value_ptr(transform.position()),
        0.2f, -FLT_MAX, FLT_MAX
    );

    // FIXME: This is slightly more usable, but the singularity for Pitch around 90d
    // is still unstable. In general: Local X is Pitch, Global Y is Yaw, and Local Z is Roll.
    // Stll very messy to use, but should get the ball rolling.
    const glm::quat& q = transform.orientation();
    // Swap quaternion axes to make pitch around (local) X axis.
    // Also GLM for some reason assumes that the locking [-90, 90] axis is
    // associated with Yaw, not Pitch...? Maybe I am confused here but
    // I want it Pitch, so we also have to swap the euler representation.
    // (In my mind, Pitch and Yaw are Theta and Phi in spherical coordinates respectively).
    const glm::quat q_shfl = glm::quat::wxyz(q.w, q.y, q.x, q.z);
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
        transform.orientation() = glm::quat::wxyz(p.w, p.y, p.x, p.z);
        feedback |= true;
    }

    feedback |= ImGui::DragFloat3(
        "Scale", glm::value_ptr(transform.scaling()),
        0.1f, 0.001f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    return feedback;
}


inline void Matrix4x4DisplayWidget(const glm::mat4& mat4) {
    const int num_rows = 4;
    const int num_cols = 4;
    ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_NoHostExtendX;
    if (ImGui::BeginTable("Matrix4x4", num_cols, flags)) {
        for (int row = 0; row < num_rows; ++row) {
            ImGui::TableNextRow();
            for (int col = 0; col < num_cols; ++col) {
                ImGui::TableSetColumnIndex(col);

                ImGui::Text("%.3f", mat4[col][row]);
            }
        }
        ImGui::EndTable();
    }
}


inline void Matrix3x3DisplayWidget(const glm::mat3& mat3) {
    const int num_rows = 3;
    const int num_cols = 3;
    ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_NoHostExtendX;
    if (ImGui::BeginTable("Matrix4x4", num_cols, flags)) {
        for (int row = 0; row < num_rows; ++row) {
            ImGui::TableNextRow();
            for (int col = 0; col < num_cols; ++col) {
                ImGui::TableSetColumnIndex(col);

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

    const float  text_height  = ImGui::GetTextLineHeight();
    const ImVec2 preview_size = { 4.f * text_height, 4.f * text_height };

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


inline void AnimationsWidget(Handle handle) {
    if (auto* anims = handle.try_get<MeshAnimations>()) {
        if (ImGui::TreeNode("Animations")) {

            for (size_t i{ 0 }; i < anims->anims.size(); ++i) {
                const auto& anim       = anims->anims[i];
                const float duration_s = float(anim->duration);
                ImGui::Text("%zu | %.2f s",  i, duration_s);
                ImGui::SameLine();
                if (ImGui::SmallButton("Play")) {
                    PlayingAnimation playing{
                        .current_time = 0.0,
                        .current_anim = anim,
                        .paused       = false,
                    };
                    handle.emplace_or_replace<PlayingAnimation>(MOVE(playing));
                }
                if (auto* playing = handle.try_get<PlayingAnimation>()) {
                    if (playing->current_anim == anim) {
                        ImGui::SameLine();
                        if (!playing->paused && ImGui::SmallButton("Pause ")) { playing->paused = true;  }
                        if (playing->paused  && ImGui::SmallButton("Resume")) { playing->paused = false; }
                        ImGui::SameLine();
                        ImGui::Text("Playing [%.2f s]", playing->current_time);
                    }
                }
            }

            ImGui::TreePop();
        }
    }
    if (auto* skinned_mesh = handle.try_get<SkinnedMesh>()) {
        if (ImGui::TreeNode("Skin Mat4s (B2J[@M])")) {
            for (const mat4& skin_mat : skinned_mesh->pose.skinning_mats) {
                Matrix4x4DisplayWidget(skin_mat);
            }
            ImGui::TreePop();
        }
    }
}



inline bool AmbientLightWidget(AmbientLight& alight) {
    bool feedback = false;
    feedback |= ImGui::ColorEdit3("Color", glm::value_ptr(alight.color), ImGuiColorEditFlags_DisplayHSV);
    feedback |= ImGui::DragFloat("Irradiance, W/m^2", &alight.irradiance, 0.1f, 0.f, FLT_MAX);
    return feedback;
}


inline bool AmbientLightHandleWidget(entt::handle alight_handle) {
    if (auto* alight = alight_handle.try_get<AmbientLight>()) {
        return AmbientLightWidget(*alight);
    }
    return false;
}


inline bool ShadowCastingHandleWidget(entt::handle light_handle) {
    bool has_shadow = has_tag<ShadowCasting>(light_handle);
    if (ImGui::Checkbox("Shadow", &has_shadow)) {
        switch_tag<ShadowCasting>(light_handle);
        return true;
    }
    return false;
}


inline bool DirectionalLightWidget(DirectionalLight& dlight) {
    bool feedback = false;
    feedback |= ImGui::ColorEdit3("Color", glm::value_ptr(dlight.color), ImGuiColorEditFlags_DisplayHSV);
    feedback |= ImGui::DragFloat("Irradiance, W/m^2", &dlight.irradiance, 0.1f, 0.f, FLT_MAX);
    return feedback;
}


inline bool DirectionalLightHandleWidget(entt::handle dlight_handle) {
    bool feedback = false;
    if (auto* dlight = dlight_handle.try_get<DirectionalLight>()) {
        feedback |= DirectionalLightWidget(*dlight);
        feedback |= ShadowCastingHandleWidget(dlight_handle);
    }
    return feedback;
}


inline bool PointLightRadiantFluxWidget(float& quadratic_attenuation) noexcept {
    bool feedback = false;
    constexpr float four_pi = 4.f * glm::pi<float>();
    float rf = four_pi / quadratic_attenuation;
    if (ImGui::DragFloat("Radiant Power, W", &rf, 0.1f, 0.f, FLT_MAX)) {
        quadratic_attenuation = four_pi / rf;
        feedback |= true;
    }
    return feedback;
}


inline bool PointLightWidget(PointLight& plight) {
    bool feedback = false;
    feedback |= ImGui::ColorEdit3("Color", glm::value_ptr(plight.color), ImGuiColorEditFlags_DisplayHSV);
    feedback |= ImGui::DragFloat("Radiant Power, W", &plight.power, 0.1f, 0.f, FLT_MAX);
    return feedback;
}


inline bool PointLightHandleWidget(entt::handle plight_handle) {
    bool feedback = false;
    if (auto* plight = plight_handle.try_get<PointLight>()) {
        feedback |= PointLightWidget(*plight);
        feedback |= ShadowCastingHandleWidget(plight_handle);
    }
    return feedback;
}


inline bool CameraHandleWidget(entt::handle camera_handle) noexcept {
    bool need_update = false;
    if (auto* camera = camera_handle.try_get<Camera>()) {
        auto params = camera->get_params();

        if (ImGui::DragFloatRange2("Z Near/Far", &params.z_near, &params.z_far,
            0.2f, 0.0001f, 10000.f, "%.4f", nullptr, ImGuiSliderFlags_Logarithmic))
        {
            need_update |= true;
        }

        float fovy_deg = glm::degrees(params.fovy_rad);
        if (ImGui::DragFloat("Y FoV, deg", &fovy_deg,
            0.2f, 0.f, FLT_MAX))
        {
            params.fovy_rad = glm::radians(fovy_deg);
            need_update |= true;
        }

        if (need_update) {
            camera->update_params(params);
        }
    }
    return need_update;
}


} // namespace josh::imgui
