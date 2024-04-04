#pragma once
#include "GLObjects.hpp"
#include "ImGuiHelpers.hpp"
#include "components/Mesh.hpp"
#include "components/Model.hpp"
#include "components/Materials.hpp"
#include "components/Name.hpp"
#include "components/Path.hpp"
#include "components/Transform.hpp"
#include "components/VPath.hpp"
#include <cassert>
#include <entt/entity/entity.hpp>
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


inline void MeshWidget(entt::handle mesh_handle) noexcept {

    ImGui::PushID(void_id(mesh_handle.entity()));

    if (auto name = mesh_handle.try_get<components::Name>()) {
        ImGui::Text("Mesh [%d]: %s", entt::to_entity(mesh_handle.entity()), name->name.c_str());
    } else {
        ImGui::Text("Mesh [%d]", entt::to_entity(mesh_handle.entity()));
    }

    if (auto tf = mesh_handle.try_get<components::Transform>()) {
        TransformWidget(tf);
    }

    if (ImGui::TreeNode("Material")) {

        // FIXME: Not sure if scaling to max size is always preferrable.
        auto imsize = [&](RawTexture2D<GLConst> tex) -> ImVec2 {
            const float w = ImGui::GetContentRegionAvail().x;
            const float h = w / tex.get_resolution().aspect_ratio();
            return { w, h };
        };

        if (auto material = mesh_handle.try_get<components::MaterialDiffuse>()) {
            if (ImGui::TreeNode("Diffuse")) {
                ImGui::Unindent();

                ImageGL(void_id(material->texture->id()), imsize(material->texture));

                ImGui::Indent();
                ImGui::TreePop();
            }
        }

        if (auto material = mesh_handle.try_get<components::MaterialSpecular>()) {
            if (ImGui::TreeNode("Specular")) {
                ImGui::Unindent();

                ImageGL(void_id(material->texture->id()), imsize(material->texture));

                ImGui::DragFloat(
                    "Shininess", &material->shininess,
                    1.0f, 0.1f, 1.e4f, "%.3f", ImGuiSliderFlags_Logarithmic
                );

                ImGui::Indent();
                ImGui::TreePop();
            }
        }

        if (auto material = mesh_handle.try_get<components::MaterialNormal>()) {
            if (ImGui::TreeNode("Normal")) {
                ImGui::Unindent();

                ImageGL(void_id(material->texture->id()), imsize(material->texture));

                ImGui::Indent();
                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    ImGui::PopID();

}


inline void ModelWidget(entt::handle model_handle) noexcept {

    ImGui::PushID(void_id(model_handle.entity()));

    if (auto name = model_handle.try_get<components::Name>()) {
        ImGui::Text("Model [%d]: %s", entt::to_entity(model_handle.entity()), name->name.c_str());
    } else {
        ImGui::Text("Model [%d]", entt::to_entity(model_handle.entity()));
    }

    if (auto tf = model_handle.try_get<components::Transform>()) {
        TransformWidget(tf);
    }

    if (auto path = model_handle.try_get<components::Path>()) {
        imgui::PathWidget(path);
    }

    if (auto vpath = model_handle.try_get<components::VPath>()) {
        imgui::VPathWidget(vpath);
    }

    if (ImGui::TreeNode("Meshes")) {

        auto model = model_handle.try_get<components::Model>();
        assert(model);

        for (auto mesh_entity : model->meshes()) {    ;
            MeshWidget({ *model_handle.registry(), mesh_entity });
        }

        ImGui::TreePop();
    }

    ImGui::PopID();

}

} // namespace josh::imgui
