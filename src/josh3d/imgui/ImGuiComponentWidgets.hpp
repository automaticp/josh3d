#pragma once
#include "components/Name.hpp"
#include "components/Transform.hpp"
#include <imgui.h>


namespace ImGui {


inline bool TransformWidget(josh::components::Transform* transform) noexcept {

    bool feedback{ false };

    feedback |= ImGui::DragFloat3(
        "Position", glm::value_ptr(transform->position()),
        0.2f, -100.f, 100.f
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
    ImGui::TextUnformatted(name->name.c_str());
}


} // namespace josh
