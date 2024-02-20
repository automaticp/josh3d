#pragma once
#include "RenderEngine.hpp"
#include "RenderStage.hpp"
#include "UniqueFunction.hpp"
#include <concepts>
#include <typeindex>
#include <utility>




namespace josh {


template<typename StageHookT>
concept specifies_target_stage = requires {
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
class ImGuiEngineHooks {
public:
    class HooksContainer {
    private:
        friend ImGuiEngineHooks;

        using Hook    = UniqueFunction<void(AnyRef)>;
        using HookMap = std::unordered_map<std::type_index, Hook>;

        HookMap hooks_;

    public:
        // TODO: Should there be `emplace_*` functions that take a `type_index`?

        // This will source the target stage type from the `StageHookT::target_stage_type` typedef.
        template<specifies_target_stage StageHookT>
            requires std::invocable<StageHookT, AnyRef>
        void add_hook(StageHookT&& hook) {
            add_hook_explicit<typename StageHookT::target_stage_type>(std::forward<StageHookT>(hook));
        }

        // This allows you to specify the concrete stage type that your hook expects.
        template<typename TargetStageT, std::invocable<AnyRef> StageHookT>
        void add_hook_explicit(StageHookT&& hook) {
            hooks_.insert_or_assign(
                std::type_index(typeid(TargetStageT)),
                std::forward<StageHookT>(hook)
            );
        }

    };

private:
    HooksContainer hooks_container_;
    RenderEngine&  engine_;

public:
    ImGuiEngineHooks(RenderEngine& engine) : engine_{ engine } {}

          HooksContainer& hooks()       noexcept { return hooks_container_; }
    const HooksContainer& hooks() const noexcept { return hooks_container_; }

    void display();
};





} // namespace josh
