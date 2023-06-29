#pragma once
#include "UniqueFunction.hpp"
#include <concepts>
#include <entt/fwd.hpp>


namespace learn {

class RenderEngine;
class RenderEnginePrimaryInterface;
class RenderEnginePostprocessInterface;


template<typename StageT>
concept primary_render_stage = std::invocable<StageT, const RenderEnginePrimaryInterface&, const entt::registry&>;

template<typename StageT>
concept postprocess_render_stage = std::invocable<StageT, const RenderEnginePostprocessInterface&, const entt::registry&>;




/*
A generic container for primary stages that preserves the type of the stored callable.
Used to separate the construction of the stage from addition to the rendering engine.
*/
template<primary_render_stage StageT>
class PrimaryStage {
private:
    UniqueFunction<void(const RenderEnginePrimaryInterface&, const entt::registry&)> stage_;

    // Only constructible from RenderEngine.
    // Could be because RenderEngine would want to do some bookkeeping.
    friend RenderEngine;
    PrimaryStage(StageT&& stage) : stage_{ std::move(stage) } {}

public:
    StageT& target() noexcept { return stage_.target_unchecked<StageT>(); }
    const StageT& target() const noexcept { return stage_.target_unchecked<StageT>(); }

    operator StageT&() noexcept { return target(); }
    operator const StageT&() const noexcept { return target(); }

};



/*
A generic container for postfx stages that preserves the type of the stored callable.
Used to separate the construction of the stage from addition to the rendering engine.
*/
template<postprocess_render_stage StageT>
class PostprocessStage {
private:
    UniqueFunction<void(const RenderEnginePostprocessInterface&, const entt::registry&)> stage_;

    // All these friendships are here, because nested class
    // definitions are a pain to work with and can't be
    // forward declared in most cases.
    friend RenderEngine;
    PostprocessStage(StageT&& stage) : stage_{ std::move(stage) } {}

public:
    StageT& target() noexcept { return stage_.target_unchecked<StageT>(); }
    const StageT& target() const noexcept { return stage_.target_unchecked<StageT>(); }

    operator StageT&() noexcept { return target(); }
    operator const StageT&() const noexcept { return target(); }

};




namespace detail {


/*
Type erased primary stage stored inside the RenderEngine stages container.
*/
class AnyPrimaryStage {
private:
    friend RenderEngine;

    using stage_t =
        UniqueFunction<void(const RenderEnginePrimaryInterface&, const entt::registry&)>;

    stage_t stage_;

    AnyPrimaryStage(stage_t&& stage) : stage_{ std::move(stage) } {}

public:
    void operator()(const RenderEnginePrimaryInterface& engine, const entt::registry& registry) {
        stage_(engine, registry);
    }
};




/*
Type erased postfx stage stored inside the RenderEngine stages container.
*/
class AnyPostprocessStage {
private:
    friend RenderEngine;

    using stage_t =
        UniqueFunction<void(const RenderEnginePostprocessInterface&, const entt::registry&)>;

    stage_t stage_;

    AnyPostprocessStage(stage_t&& stage) : stage_{ std::move(stage) } {}

public:
    void operator()(const RenderEnginePostprocessInterface& engine, const entt::registry& registry) {
        stage_(engine, registry);
    }
};




} // namespace detail




} // namespace learn
