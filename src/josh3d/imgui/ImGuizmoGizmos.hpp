#pragma once
#include "Camera.hpp"
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


enum class GizmoLocation {
    LocalOrigin,
    AABBMidpoint
};


class ImGuizmoGizmos {
public:
    GizmoOperation active_operation    { GizmoOperation::Translation };
    GizmoSpace     active_space        { GizmoSpace::World           };
    GizmoLocation  preferred_location  { GizmoLocation::AABBMidpoint };
    bool           display_debug_window{ false                       };

    ImGuizmoGizmos(entt::registry& registry) : registry_{ registry } {}

    void new_frame();
    void display(const glm::mat4& view_mat, const glm::mat4& proj_mat);

private:
    entt::registry& registry_;
};


} // namespace josh
