#pragma once
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "UIContextFwd.hpp"
#include "UniqueFunction.hpp"
#include <concepts>
#include <typeindex>


namespace josh {

template<typename StageHookT>
concept specifies_target_stage = requires
{
    typename StageHookT::target_stage_type;
};

/*
A container for various ImGui code that can be injected into
a general Render Stages debug window.

[Render Stages]
  [Primary]
    [Stage Name 1]
      <Your hook here>
    [Stage Name 2]
      <Your hook here>
    ...
  [Postprocessing]
    [Stage 1]
      <Your hook here>
    ...

*/
struct ImGuiEngineHooks
{
    void display(UIContext& ui);

    // This will source the target stage type from the `StageHookT::target_stage_type` typedef.
    // HMM: Why would this be a good idea at all?
    template<specifies_target_stage StageHookT>
        requires std::invocable<StageHookT, AnyRef>
    void add_hook(StageHookT&& hook)
    {
        add_hook_explicit<typename StageHookT::target_stage_type>(FORWARD(hook));
    }

    // This allows you to specify the concrete stage type that your hook expects.
    template<typename TargetStageT, std::invocable<AnyRef> StageHookT>
    void add_hook_explicit(StageHookT&& hook)
    {
        _hooks.insert_or_assign(std::type_index(typeid(TargetStageT)), FORWARD(hook));
    }

    using hook_map = HashMap<std::type_index, UniqueFunction<void(AnyRef)>>;
    hook_map _hooks;
};


} // namespace josh
