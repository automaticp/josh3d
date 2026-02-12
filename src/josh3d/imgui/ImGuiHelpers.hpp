#pragma once
#include "CategoryCasts.hpp"
#include <concepts>
#include <type_traits>
#include <utility>
#include <imgui.h>


namespace josh {


inline auto void_id(std::integral auto value) noexcept
    -> void*
{
    return reinterpret_cast<void*>(value); // NOLINT
}

template<typename EnumT>
    requires std::is_enum_v<EnumT>
inline auto void_id(EnumT value) noexcept
    -> void*
{
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
class OnValueCondition
{
public:
    void set(ValueT signal_value) { value_ = MOVE(signal_value); }
    OnValueCondition(const OnValueCondition&) = delete;
    OnValueCondition(OnValueCondition&&) = delete;
    OnValueCondition& operator=(const OnValueCondition&) = delete;
    OnValueCondition& operator=(OnValueCondition&&) = delete;
    ~OnValueCondition() noexcept(noexcept(reset())) { reset(); }

private:
    ValueT        value_;
    ConditionFunT condition_;
    ResetFunT     reset_fun_;

    template<typename ValT, typename ConditionF, typename ResetF>
    friend auto on_value_condition(ValT value, ConditionF&& condition, ResetF&& reset_fun)
        -> OnValueCondition<ValT, ConditionF, ResetF>;

    OnValueCondition(ValueT value, ConditionFunT&& condition, ResetFunT&& reset_fun)
        : value_    { MOVE(value) }
        , condition_{ FORWARD(condition) }
        , reset_fun_{ FORWARD(reset_fun) }
    {}

    void reset()
        requires std::invocable<ResetFunT, ValueT&>
    {
        if (FORWARD(condition_)(std::as_const(value_)))
            FORWARD(reset_fun_)(value_);
    }

    void reset()
        requires std::invocable<ResetFunT>
    {
        if (FORWARD(condition_)(std::as_const(value_)))
            FORWARD(reset_fun_)();
    }
};


/*
Factory constructor that preserves value category as opposed to CTAD of OnValueCondition.
*/
template<typename ValT, typename ConditionF, typename ActionF>
[[nodiscard]] auto on_value_condition(ValT initial_value, ConditionF&& condition, ActionF&& reset_fun)
    -> OnValueCondition<ValT, ConditionF, ActionF>
{
    return {
        MOVE(initial_value),
        FORWARD(condition),
        FORWARD(reset_fun)
    };
}

/*
OnValueCondition that triggers when the value changed from initial sentinel.
*/
template<typename ValT, typename ActionF>
[[nodiscard]] auto on_value_change_from(ValT sentinel_value, ActionF&& action_fun) {
    return on_value_condition(
        MOVE(sentinel_value),
        [=](const ValT& val) { return val != sentinel_value; },
        FORWARD(action_fun)
    );
}

/*
OnValueCondition that triggers when the signal is set to `true`.
Initially disengaged.
*/
template<typename ActionF>
[[nodiscard]] auto on_signal(ActionF&& action)
{
    return on_value_change_from(false, FORWARD(action));
}


} // namespace josh
