#pragma once
#include "PerspectiveCamera.hpp"
#include <entt/entity/registry.hpp>


namespace josh {


enum class GizmoOperation {
    translation,
    rotation,
    scaling
};


enum class GizmoSpace {
    world,
    local
};


class ImGuizmoGizmos {
private:
    const PerspectiveCamera& cam_;
    entt::registry& registry_;

public:
    GizmoOperation active_operation{ GizmoOperation::translation };
    GizmoSpace     active_space    { GizmoSpace::world            };

    ImGuizmoGizmos(const PerspectiveCamera& ref_camera, entt::registry& registry)
        : cam_     { ref_camera }
        , registry_{ registry   }
    {}

    void new_frame();
    void display();
};


} // namespace josh
