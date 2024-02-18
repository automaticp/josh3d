#pragma once
#include "UniqueFunction.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/fwd.hpp>
#include <concepts>


namespace josh {


class RenderEngine;
class RenderEnginePrecomputeInterface;
class RenderEnginePrimaryInterface;
class RenderEnginePostprocessInterface;
class RenderEngineOverlayInterface;


template<typename StageT>
concept precompute_render_stage =
    std::invocable<StageT, const RenderEnginePrecomputeInterface&, entt::registry&>;

template<typename StageT>
concept primary_render_stage =
    std::invocable<StageT, const RenderEnginePrimaryInterface&, const entt::registry&>;

template<typename StageT>
concept postprocess_render_stage =
    std::invocable<StageT, const RenderEnginePostprocessInterface&, const entt::registry&>;

template<typename StageT>
concept overlay_render_stage =
    std::invocable<StageT, const RenderEngineOverlayInterface&, const entt::registry&>;




/*
A generic container for precompute stages that preserves the type of the stored callable.
Used to separate the construction of the stage from addition to the rendering engine.

Most notably, the registry is mutable in this stage.
*/
template<precompute_render_stage StageT>
class PrecomputeStage {
private:
    UniqueFunction<void(const RenderEnginePrecomputeInterface&, entt::registry&)> stage_;

    friend RenderEngine;
    PrecomputeStage(StageT&& stage) : stage_{ std::move(stage) } {}

public:
    StageT& target() noexcept { return stage_.target_unchecked<StageT>(); }
    const StageT& target() const noexcept { return stage_.target_unchecked<StageT>(); }

    operator StageT&() noexcept { return target(); }
    operator const StageT&() const noexcept { return target(); }

};




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




/*
A generic container for overlay stages that preserves the type of the stored callable.
Used to separate the construction of the stage from addition to the rendering engine.
*/
template<overlay_render_stage StageT>
class OverlayStage {
private:
    UniqueFunction<void(const RenderEngineOverlayInterface&, const entt::registry&)> stage_;

    friend RenderEngine;
    OverlayStage(StageT&& stage) : stage_{ std::move(stage) } {}

public:
    StageT& target() noexcept { return stage_.target_unchecked<StageT>(); }
    const StageT& target() const noexcept { return stage_.target_unchecked<StageT>(); }

    operator StageT&() noexcept { return target(); }
    operator const StageT&() const noexcept { return target(); }

};




namespace detail {




/*
Type erased precompute stage stored inside the RenderEngine stages container.
*/
class AnyPrecomputeStage {
private:
    friend RenderEngine;

    using stage_t =
        UniqueFunction<void(const RenderEnginePrecomputeInterface&, entt::registry&)>;

    stage_t stage_;

    AnyPrecomputeStage(stage_t&& stage) : stage_{ std::move(stage) } {}

public:
    void operator()(const RenderEnginePrecomputeInterface& engine, entt::registry& registry) {
        stage_(engine, registry);
    }
};




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




/*
Type erased overlay stage stored inside the RenderEngine stages container.
*/
class AnyOverlayStage {
private:
    friend RenderEngine;

    using stage_t =
        UniqueFunction<void(const RenderEngineOverlayInterface&, const entt::registry&)>;

    stage_t stage_;

    AnyOverlayStage(stage_t&& stage) : stage_{ std::move(stage) } {}
public:
    void operator()(const RenderEngineOverlayInterface& engine, const entt::registry& registry) {
        stage_(engine, registry);
    }
};




} // namespace detail


} // namespace josh
