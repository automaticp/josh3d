#pragma once
#include "PerspectiveCamera.hpp"
#include <entt/fwd.hpp>


namespace josh {


enum class GizmoOperation {
    Translation,
    Rotation,
    Scaling
};


enum class GizmoSpace {
    World,
    Local
};


class ImGuizmoGizmos {
public:
    GizmoOperation active_operation    { GizmoOperation::Translation };
    GizmoSpace     active_space        { GizmoSpace::World           };
    bool           display_debug_window{ false                       };

    ImGuizmoGizmos(const PerspectiveCamera& ref_camera, entt::registry& registry)
        : cam_     { ref_camera }
        , registry_{ registry   }
    {}

    void new_frame();
    void display();

private:
    const PerspectiveCamera& cam_;
    entt::registry&          registry_;
};


} // namespace josh
