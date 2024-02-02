#pragma once
#include "PerspectiveCamera.hpp"
#include <entt/entity/registry.hpp>


namespace josh {


class ImGuizmoGizmos {
private:
    const PerspectiveCamera& cam_;
    entt::registry& registry_;

public:
    ImGuizmoGizmos(const PerspectiveCamera& ref_camera, entt::registry& registry)
        : cam_     { ref_camera }
        , registry_{ registry   }
    {}

    void new_frame();
    void display();
};


} // namespace josh
