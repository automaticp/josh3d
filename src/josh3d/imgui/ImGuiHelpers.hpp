#pragma once
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


} // namespace ImGui
