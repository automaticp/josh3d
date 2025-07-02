#pragma once
#include "AABB.hpp"
#include "Active.hpp"
#include "BoundingSphere.hpp"
#include "Camera.hpp"
#include "Components.hpp"
#include "DefaultTextures.hpp"
#include "ECS.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "ImGuiExtras.hpp"
#include "LightCasters.hpp"
#include "Mesh.hpp"
#include "Materials.hpp"
#include "Name.hpp"
#include "Ranges.hpp"
#include "SceneGraph.hpp"
#include "ScopeExit.hpp"
#include "SkeletalAnimation.hpp"
#include "SkinnedMesh.hpp"
#include "Skybox.hpp"
#include "Tags.hpp"
#include "Filesystem.hpp"
#include "TerrainChunk.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "AlphaTested.hpp"
#include "ShadowCasting.hpp"
#include "Visible.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <cassert>


namespace josh::imgui {


struct GenericHeaderInfo
{
    const char* type_name = "UnknownEntityType";
    const char* name      = "";
};

inline auto GetGenericHeaderInfo(CHandle handle)
    -> GenericHeaderInfo
{
    const char* type_name = eval%[&]{
        if (has_component<Mesh>            (handle)) return "Mesh";
        if (has_component<SkinnedMesh>     (handle)) return "SkinnedMesh";
        if (has_component<TerrainChunk>    (handle)) return "TerrainChunk";
        if (has_component<AmbientLight>    (handle)) return "AmbientLight";
        if (has_component<DirectionalLight>(handle)) return "DirectionalLight";
        if (has_component<PointLight>      (handle)) return "PointLight";
        if (has_component<Skybox>          (handle)) return "Skybox";
        if (has_component<Camera>          (handle)) return "Camera";

        if (has_component<Transform>(handle))
        {
            if (has_children(handle)) return "Node";
            else                      return "Orphan";
        }
        else
        {
            if (has_children(handle)) return "GroupingNode"; // Does this even make sense?
            else                      return "UnknownEntity";
        }
    };

    const char* name = eval%[&]{
        if (const auto* name = handle.try_get<Name>())
            return name->c_str();
        return "";
    };

    return { type_name, name };
}

inline auto GetGenericVisibility(CHandle handle)
    -> bool
{
    if (has_component<AmbientLight>    (handle)) return is_active<AmbientLight>    (handle);
    if (has_component<DirectionalLight>(handle)) return is_active<DirectionalLight>(handle);
    if (has_component<Skybox>          (handle)) return is_active<Skybox>          (handle);
    if (has_component<Camera>          (handle)) return is_active<Camera>          (handle);

    if (handle.any_of<AABB, BoundingSphere>())
        return has_tag<Visible>(handle);

    return true;
}

inline void GenericHeaderText(CHandle handle)
{
    const bool is_visible = GetGenericVisibility(handle);

    if (not is_visible)
    {
        auto text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        text_color.w *= 0.5f; // Dim text when culled.
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    }

    auto [type_name, name] = GetGenericHeaderInfo(handle);
    ImGui::Text("[%d] [%s] %s", to_entity(handle.entity()), type_name, name);

    if (not is_visible)
        ImGui::PopStyleColor();
}

struct GenericActiveInfo
{
    bool can_be_active = false;
    bool is_active     = false;
};

inline auto GetGenericActiveInfo(CHandle handle)
    -> GenericActiveInfo
{
    if (has_component<AmbientLight>    (handle)) return { true, is_active<AmbientLight>    (handle) };
    if (has_component<DirectionalLight>(handle)) return { true, is_active<DirectionalLight>(handle) };
    if (has_component<Skybox>          (handle)) return { true, is_active<Skybox>          (handle) };
    if (has_component<Camera>          (handle)) return { true, is_active<Camera>          (handle) };
    return { false, false };
}

inline void GenericMakeActive(Handle handle)
{
    if (has_component<AmbientLight>    (handle)) make_active<AmbientLight>    (handle);
    if (has_component<DirectionalLight>(handle)) make_active<DirectionalLight>(handle);
    if (has_component<Skybox>          (handle)) make_active<Skybox>          (handle);
    if (has_component<Camera>          (handle)) make_active<Camera>          (handle);
}

inline auto TransformWidget(josh::Transform& transform) noexcept
    -> bool
{
    bool feedback = false;

    feedback |= ImGui::DragFloat3(
        "Position", value_ptr(transform.position()),
        0.2f, -FLT_MAX, FLT_MAX);

    // FIXME: This is slightly more usable, but the singularity for Pitch around 90d
    // is still unstable. In general: Local X is Pitch, Global Y is Yaw, and Local Z is Roll.
    // Stll very messy to use, but should get the ball rolling.
    const quat& q = transform.orientation();
    // Swap quaternion axes to make pitch around (local) X axis.
    // Also GLM for some reason assumes that the locking [-90, 90] axis is
    // associated with Yaw, not Pitch...? Maybe I am confused here but
    // I want it Pitch, so we also have to swap the euler representation.
    // (In my mind, Pitch and Yaw are Theta and Phi in spherical coordinates respectively).
    const quat q_shfl = quat::wxyz(q.w, q.y, q.x, q.z);
    vec3 euler = glm::degrees(vec3{
        glm::yaw(q_shfl),   // Pitch
        glm::pitch(q_shfl), // Yaw
        glm::roll(q_shfl)   // Roll
        // Dont believe what GLM says
    });

    if (ImGui::DragFloat3("Pitch/Yaw/Roll", value_ptr(euler),
        1.0f, -360.f, 360.f, "%.3f"))
    {
        euler.x = glm::clamp(euler.x, -89.999f, 89.999f);
        euler.y = glm::mod(euler.y, 360.f);
        euler.z = glm::mod(euler.z, 360.f);
        // Un-shuffle back both the euler angles and quaternions.
        quat p = quat(glm::radians(vec3{ euler.y, euler.x, euler.z }));
        transform.orientation() = quat::wxyz(p.w, p.y, p.x, p.z);
        feedback |= true;
    }

    feedback |= ImGui::DragFloat3("Scale", value_ptr(transform.scaling()),
        0.1f, 0.001f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic);

    return feedback;
}

inline void Matrix4x4DisplayWidget(const mat4& mat4)
{
    const usize num_rows = 4;
    const usize num_cols = 4;
    const ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_NoHostExtendX;

    if (ImGui::BeginTable("Matrix4x4", num_cols, flags))
    {
        for (const uindex row : irange(num_rows))
        {
            ImGui::TableNextRow();
            for (const uindex col : irange(num_cols))
            {
                ImGui::TableSetColumnIndex(int(col));
                ImGui::Text("%.3f", mat4[col][row]);
            }
        }
        ImGui::EndTable();
    }
}

template<entity_tag TagT>
auto TagCheckbox(const char* label, Handle handle)
    -> bool
{
    bool tagged = has_tag<TagT>(handle);
    if (ImGui::Checkbox(label, &tagged))
    {
        if (tagged) set_tag<TagT>(handle);
        else        unset_tag<TagT>(handle);
        return true;
    }
    return false;
}

inline void Matrix3x3DisplayWidget(const mat3& mat3)
{
    const usize num_rows = 3;
    const usize num_cols = 3;
    ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_NoHostExtendX;

    if (ImGui::BeginTable("Matrix4x4", num_cols, flags))
    {
        for (const uindex row : irange(num_rows))
        {
            ImGui::TableNextRow();
            for (const uindex col : irange(num_cols))
            {
                ImGui::TableSetColumnIndex(int(col));
                ImGui::Text("%.3f", mat3[col][row]);
            }
        }
        ImGui::EndTable();
    }
}

inline void NameWidget(josh::Name* name) noexcept
{
    ImGui::Text("Name: %s", name->c_str());
}

inline void PathWidget(josh::Path* path) noexcept
{
    ImGui::Text("Path: %s", path->c_str());
}

inline void VPathWidget(josh::VPath* vpath) noexcept
{
    ImGui::Text("VPath: %s", vpath->path().c_str());
}

inline void MaterialsWidget(Handle handle)
{
    const float  text_height  = ImGui::GetTextLineHeight();
    const ImVec2 preview_size = { 4.f * text_height, 4.f * text_height };

    const ImVec4 tex_tint         = { 1.f, 1.f, 1.f, 1.f };
    const ImVec4 tex_frame_color  = { .5f, .5f, .5f, .5f };
    const ImVec4 empty_slot_color = { .8f, .8f, .2f, .8f };
    const ImVec4 empty_slot_tint  = { 1.f, 1.f, 1.f, .2f };

    const auto slot_widget_base = [&](
        RawTexture2D<GLConst> texture,
        const char*           slot_name,
        const char*           annotation,
        const char*           popup_str_id,
        const ImVec4&         frame_color,
        const ImVec4&         tint_color,
        auto&&                extra_func)
    {
        // NOTE: Have to do this in the new setup.
        ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, 1.0f);
        DEFER(ImGui::PopStyleVar());

        const auto tex_id = texture.id();
        ImGui::ImageGL(tex_id, preview_size, tint_color, frame_color);
        // All hover and click tests below are for this image above ^.

        const auto hover_widget = [&]()
        {
            const Extent2I resolution = texture.get_resolution();
            const float    aspect     = resolution.aspect_ratio();

            const float largest_side = 512.f; // Desired.

            const ImVec2 hovered_size = eval%[&]() -> ImVec2 {
                const auto [w, h] = resolution;
                if (w == h)           return { largest_side,          largest_side          };
                else if (w < h)       return { largest_side * aspect, largest_side          };
                else /* if (w > h) */ return { largest_side,          largest_side / aspect };
            };

            ImGui::ImageGL(tex_id, hovered_size, tex_tint, frame_color);
            if (annotation) ImGui::Text("%dx%d, %s %s", resolution.width, resolution.height, slot_name, annotation);
            else            ImGui::Text("%dx%d, %s",    resolution.width, resolution.height, slot_name);

            // NOTE: The extra_func is invoked at the end of the hover widget.
            extra_func();
        };

        auto bg_col = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
        bg_col.w = .6f; // Make less opaque
        auto border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
        border_col.x *= 1.6f; // Lighten
        border_col.y *= 1.6f;
        border_col.z *= 1.6f;

        ImGui::PushStyleColor(ImGuiCol_PopupBg, bg_col);
        ImGui::PushStyleColor(ImGuiCol_Border,  border_col);
        DEFER(ImGui::PopStyleColor(2));

        // TODO: This clickable pop-up could come in handy in other places.
        // Can we make a generalization of this?

        // We want the popup to appear seamlessly, replacing the tooltip,
        // so we store the position of the tooltip window.
        ImVec2 tooltip_pos{};

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            tooltip_pos = ImGui::GetWindowPos();
            hover_widget();
            ImGui::EndTooltip();
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            ImGui::SetNextWindowPos(tooltip_pos);
            ImGui::OpenPopup(popup_str_id);
        }

        if (ImGui::BeginPopup(popup_str_id))
        {
            hover_widget();
            ImGui::EndPopup();
        }
    };

    // Wrapper of above that handles default textures itself.
    const auto slot_widget = [&](
        RawTexture2D<GLConst> texture,
        RawTexture2D<GLConst> default_texture,
        const char*           slot_name,
        auto&&                extra_func)
    {
        const bool is_default =
            texture.id() == default_texture.id();

        const ImVec4 frame_color = is_default ? empty_slot_color : tex_frame_color;
        const ImVec4 tint        = is_default ? empty_slot_tint  : tex_tint;

        const char* annotation = is_default ? "(Default)" : nullptr;

        // NOTE: Passing slot_name as popup_str_id. Do not name 2 slots the same way.
        slot_widget_base(texture, slot_name, annotation, slot_name, frame_color, tint, FORWARD(extra_func));
    };

    if (auto mtl = handle.try_get<MaterialPhong>())
    {
        slot_widget(mtl->diffuse, globals::default_diffuse_texture(), "Diffuse", []{});

        // Extra specpower control for the specular slot.
        const auto specpower_widget = [&]()
        {
            ImGui::DragFloat("Specpower", &mtl->specpower, 1.f, 0.1f, 8192.f, "%.2f", ImGuiSliderFlags_Logarithmic);
        };

        ImGui::SameLine();
        slot_widget(mtl->specular, globals::default_specular_texture(), "Specular", specpower_widget);

        ImGui::SameLine();
        slot_widget(mtl->normal, globals::default_normal_texture(), "Normals", []{});

        const bool can_be_alpha_tested =
            mtl->diffuse->get_component_type<PixelComponent::Alpha>() != PixelComponentType::None;

        // HMM: Could be a diffuse widget extra instead, but will require more clicks to adjust.
        if (can_be_alpha_tested)
        {
            ImGui::SameLine();
            TagCheckbox<AlphaTested>("Alpha-Testing", handle);
        }
    }
}

inline void AnimationsWidget(Handle handle)
{
    if (auto* anims = handle.try_get<MeshAnimations>())
    {
        if (ImGui::TreeNode("Animations"))
        {
            for (const uindex i : irange(anims->anims.size()))
            {
                const auto& anim       = anims->anims[i];
                const float duration_s = float(anim->duration);

                ImGui::Text("%zu | %.2f s",  i, duration_s);
                ImGui::SameLine();
                if (ImGui::SmallButton("Play"))
                {
                    PlayingAnimation playing = {
                        .current_time = 0.0,
                        .current_anim = anim,
                        .paused       = false,
                    };
                    handle.emplace_or_replace<PlayingAnimation>(MOVE(playing));
                }

                if (auto* playing = handle.try_get<PlayingAnimation>())
                {
                    if (playing->current_anim == anim)
                    {
                        ImGui::SameLine();
                        if (not playing->paused and ImGui::SmallButton("Pause ")) playing->paused = true;
                        if (playing->paused     and ImGui::SmallButton("Resume")) playing->paused = false;
                        ImGui::SameLine();
                        ImGui::Text("Playing [%.2f s]", playing->current_time);
                    }
                }
            }

            ImGui::TreePop();
        }
    }

    if (auto* skinned_mesh = handle.try_get<SkinnedMesh>())
    {
        if (ImGui::TreeNode("Skin Mat4s (B2J[@M])"))
        {
            for (const mat4& skin_mat : skinned_mesh->pose.skinning_mats)
                Matrix4x4DisplayWidget(skin_mat);
            ImGui::TreePop();
        }
    }
}

inline auto AmbientLightWidget(AmbientLight& alight)
    -> bool
{
    bool feedback = false;
    feedback |= ImGui::ColorEdit3("Color", value_ptr(alight.color), ImGuiColorEditFlags_DisplayHSV);
    feedback |= ImGui::DragFloat("Irradiance, W/m^2", &alight.irradiance, 0.1f, 0.f, FLT_MAX);
    return feedback;
}

inline auto AmbientLightHandleWidget(Handle alight_handle)
    -> bool
{
    if (auto* alight = alight_handle.try_get<AmbientLight>())
        return AmbientLightWidget(*alight);
    return false;
}

inline auto ShadowCastingHandleWidget(Handle light_handle)
    -> bool
{
    return TagCheckbox<ShadowCasting>("Shadow", light_handle);
}

inline auto DirectionalLightWidget(DirectionalLight& dlight)
    -> bool
{
    bool feedback = false;
    feedback |= ImGui::ColorEdit3("Color", glm::value_ptr(dlight.color), ImGuiColorEditFlags_DisplayHSV);
    feedback |= ImGui::DragFloat("Irradiance, W/m^2", &dlight.irradiance, 0.1f, 0.f, FLT_MAX);
    return feedback;
}

inline auto DirectionalLightHandleWidget(entt::handle dlight_handle)
    -> bool
{
    bool feedback = false;
    if (auto* dlight = dlight_handle.try_get<DirectionalLight>())
    {
        feedback |= DirectionalLightWidget(*dlight);
        feedback |= ShadowCastingHandleWidget(dlight_handle);
    }
    return feedback;
}

inline auto PointLightRadiantFluxWidget(float& quadratic_attenuation) noexcept
    -> bool
{
    bool feedback = false;
    constexpr float four_pi = 4.f * glm::pi<float>();
    float rf = four_pi / quadratic_attenuation;
    if (ImGui::DragFloat("Radiant Power, W", &rf, 0.1f, 0.f, FLT_MAX))
    {
        quadratic_attenuation = four_pi / rf;
        feedback |= true;
    }
    return feedback;
}

inline auto PointLightWidget(PointLight& plight)
    -> bool
{
    bool feedback = false;
    feedback |= ImGui::ColorEdit3("Color", glm::value_ptr(plight.color), ImGuiColorEditFlags_DisplayHSV);
    feedback |= ImGui::DragFloat("Radiant Power, W", &plight.power, 0.1f, 0.f, FLT_MAX);
    return feedback;
}

inline auto PointLightHandleWidget(entt::handle plight_handle)
    -> bool
{
    bool feedback = false;
    if (auto* plight = plight_handle.try_get<PointLight>())
    {
        feedback |= PointLightWidget(*plight);
        feedback |= ShadowCastingHandleWidget(plight_handle);
    }
    return feedback;
}

inline auto CameraHandleWidget(entt::handle camera_handle) noexcept
    -> bool
{
    bool update = false;
    if (auto* camera = camera_handle.try_get<Camera>())
    {
        auto params = camera->get_params();

        update |= ImGui::DragFloatRange2("Z Near/Far", &params.z_near, &params.z_far,
            0.2f, 0.0001f, 10000.f, "%.4f", nullptr, ImGuiSliderFlags_Logarithmic);

        float fovy_deg = glm::degrees(params.fovy_rad);
        if (ImGui::DragFloat("Y FoV, deg", &fovy_deg, 0.2f, 0.f, FLT_MAX))
        {
            params.fovy_rad = glm::radians(fovy_deg);
            update |= true;
        }

        if (update)
            camera->update_params(params);
    }
    return update;
}


} // namespace josh::imgui
