#pragma once
#include "Transform.hpp"
#include <concepts>
#include <type_traits>
#include <functional>
#include <utility>
#include <imgui.h>


namespace josh {


inline void* void_id(std::integral auto value) noexcept {
    return reinterpret_cast<void*>(value);
}

template<typename EnumT>
    requires std::is_enum_v<EnumT>
inline void* void_id(EnumT value) noexcept {
    return void_id(static_cast<std::underlying_type_t<EnumT>>(value));
}



/*
An RAII-enabled "if statement wrapper".

Serves as a maybe-useful tool where a value change is a signalling condition
for some kind of action. Here's an example using `on_value_change()`:

    void imgui_widget(std::list<Thing>& list) {

        if (ImGui::Begin("Things")) {
            auto to_remove = on_value_change(
                list.cend(),                 // Sentinel, if value changes to something else, then calls:
                [&](const auto& remove_it) { // Called in the destructor of OnValueCodition.
                    list.erase(remove_it);
                }
            );

            for (auto it = list.begin(); it != list.end(); ++it) {
                // ...

                if (ImGui::Button("Remove Me")) {
                    // Signal by changing the value to non-sentinel.
                    to_remove.set(it);
                }

                // ...
            }

            // Will check if the value changed and do list.erase() if it did.
        }
        ImGui::End();

    }

Similar to SCOPE_EXIT and other scope guards, with a specific purpose.

Useful if you want to insert/remove elements, but the container
does not allow you to do that when iterating over it.
*/
template<typename ValueT, typename ConditionFunT, typename ResetFunT>
    requires std::convertible_to<std::invoke_result_t<ConditionFunT, const ValueT&>, bool>
class OnValueCondition {
private:
    ValueT value_;
    ConditionFunT condition_;
    ResetFunT reset_fun_;

    template<typename ValT, typename ConditionF, typename ResetF>
    friend auto on_value_condition(ValT value, ConditionF&& condition, ResetF&& reset_fun)
        -> OnValueCondition<ValT, ConditionF, ResetF>;

    OnValueCondition(ValueT value, ConditionFunT&& condition, ResetFunT&& reset_fun)
        : value_{ std::move(value) }
        , condition_{ std::forward<ConditionFunT>(condition) }
        , reset_fun_{ std::forward<ResetFunT>(reset_fun) }
    {}

public:
    void set(ValueT signal_value)
        noexcept(noexcept(value_ = std::move(signal_value)))
    {
        value_ = std::move(signal_value);
    }

    OnValueCondition(const OnValueCondition&) = delete;
    OnValueCondition(OnValueCondition&&) = delete;
    OnValueCondition& operator=(const OnValueCondition&) = delete;
    OnValueCondition& operator=(OnValueCondition&&) = delete;

    ~OnValueCondition() noexcept(noexcept(reset())) { reset(); }

private:
    void reset()
        noexcept(noexcept(std::invoke(condition_, value_)) && noexcept(std::invoke(reset_fun_, value_)))
        requires std::invocable<ResetFunT, ValueT&>
    {
        if (std::invoke(std::forward<ConditionFunT>(condition_), std::as_const(value_))) {
            std::invoke(std::forward<ResetFunT>(reset_fun_), value_);
        }
    }

    void reset()
        noexcept(noexcept(std::invoke(condition_, value_)) && noexcept(std::invoke(reset_fun_)))
        requires std::invocable<ResetFunT>
    {
        if (std::invoke(std::forward<ConditionFunT>(condition_), std::as_const(value_))) {
            std::invoke(std::forward<ResetFunT>(reset_fun_));
        }
    }

};




// Factory constructor that preserves value category
// as opposed to CTAD of OnValueCondition.
template<typename ValT, typename ConditionF, typename ActionF>
[[nodiscard]] auto on_value_condition(ValT initial_value, ConditionF&& condition, ActionF&& reset_fun)
    -> OnValueCondition<ValT, ConditionF, ActionF>
{
    return {
        std::move(initial_value),
        std::forward<ConditionF>(condition),
        std::forward<ActionF>(reset_fun)
    };
}


// OnValueCondition that triggers when the value changed
// from initial sentinel.
template<typename ValT, typename ActionF>
[[nodiscard]] auto on_value_change_from(ValT sentinel_value, ActionF&& action_fun) {
    return on_value_condition(
        std::move(sentinel_value),
        [=](const ValT& val) { return val != sentinel_value; },
        std::forward<ActionF>(action_fun)
    );
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


inline bool TransformWidget(josh::Transform* transform) noexcept {

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


} // namespace ImGui
