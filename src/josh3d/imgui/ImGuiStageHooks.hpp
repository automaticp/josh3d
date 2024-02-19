#pragma once
#include "RenderEngine.hpp"
#include "RenderStage.hpp"
#include "UniqueFunction.hpp"
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

TODO: Rename to ImGuiEngineHooks or similar.
*/
class ImGuiStageHooks {
public:
    class HooksContainer {
    private:
        friend ImGuiStageHooks;

        template<typename AnyStageT>
        using Hook = UniqueFunction<void(AnyStageT&)>;
        template<typename AnyStageT>
        using HookMap = std::unordered_map<std::type_index, Hook<AnyStageT>>;

        HookMap<AnyPrecomputeStage>  precompute_hooks_;
        HookMap<AnyPrimaryStage>     primary_hooks_;
        HookMap<AnyPostprocessStage> postprocess_hooks_;
        HookMap<AnyOverlayStage>     overlay_hooks_;

    public:
        // TODO: Add interface that allows to explicitly specify target stage type.

        template<specifies_target_stage StageHookT>
        void add_precompute_hook(StageHookT&& hook) {
            precompute_hooks_.insert_or_assign(
                std::type_index(typeid(typename StageHookT::target_stage_type)),
                std::forward<StageHookT>(hook)
            );
        }

        template<specifies_target_stage StageHookT>
        void add_primary_hook(StageHookT&& hook) {
            primary_hooks_.insert_or_assign(
                std::type_index(typeid(typename StageHookT::target_stage_type)),
                std::forward<StageHookT>(hook)
            );
        }

        template<specifies_target_stage StageHookT>
        void add_postprocess_hook(StageHookT&& hook) {
            postprocess_hooks_.insert_or_assign(
                std::type_index(typeid(typename StageHookT::target_stage_type)),
                std::forward<StageHookT>(hook)
            );
        }

        template<specifies_target_stage StageHookT>
        void add_overlay_hook(StageHookT&& hook) {
            overlay_hooks_.insert_or_assign(
                std::type_index(typeid(typename StageHookT::target_stage_type)),
                std::forward<StageHookT>(hook)
            );
        }

    };

private:
    HooksContainer hooks_container_;
    RenderEngine&  engine_;

public:
    ImGuiStageHooks(RenderEngine& engine) : engine_{ engine } {}

    HooksContainer&       hooks() noexcept       { return hooks_container_; }
    const HooksContainer& hooks() const noexcept { return hooks_container_; }

    void display();
};





} // namespace josh
