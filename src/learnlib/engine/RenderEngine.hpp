#pragma once
#include "Camera.hpp"
#include "UniqueFunction.hpp"
#include "WindowSize.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>
#include <vector>


// WIP



namespace learn {


class RenderEngine {
public:
    using render_stage_t = UniqueFunction<void(const RenderEngine&, entt::registry&)>;
    using postprocess_stage_t = UniqueFunction<void(const RenderEngine&, entt::registry&)>;

private:
    entt::registry& registry_;

    Camera& cam_;

    std::vector<render_stage_t> stages_;
    // std::vector<postprocess_stage_t> pp_stages_;
    const WindowSize<int>& window_size_;

public:
    RenderEngine(entt::registry& registry, Camera& cam, const WindowSize<int>& window_size)
        : registry_{ registry }
        , cam_{ cam }
        , window_size_{ window_size }
    {}

    void render() {
        render_primary_stages();
    }

    auto& stages() noexcept { return stages_; }
    const auto& stages() const noexcept { return stages_; }

    auto& camera() noexcept { return cam_; }
    const auto& camera() const noexcept { return cam_; }

    const auto& window_size() const noexcept { return window_size_; }

private:
    void render_primary_stages() {
        for (auto& stage : stages_) {
            stage(*this, registry_);
        }
    }

};




} // namespace learn
