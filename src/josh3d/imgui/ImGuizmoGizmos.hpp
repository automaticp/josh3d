#pragma once
#include "ECS.hpp"
#include "EnumUtils.hpp"
#include "Skeleton.hpp"
#include <entt/fwd.hpp>


namespace josh {


enum class GizmoOperation
{
    Translation,
    Rotation,
    Scaling
};
JOSH3D_DEFINE_ENUM_EXTRAS(GizmoOperation, Translation, Rotation, Scaling);

enum class GizmoSpace
{
    World,
    Local
};
JOSH3D_DEFINE_ENUM_EXTRAS(GizmoSpace, World, Local);

enum class GizmoLocation
{
    LocalOrigin,
    AABBMidpoint
};
JOSH3D_DEFINE_ENUM_EXTRAS(GizmoLocation, LocalOrigin, AABBMidpoint);

struct ImGuizmoGizmos
{
    Registry& registry;

    GizmoOperation active_operation     = GizmoOperation::Translation;
    GizmoSpace     active_space         = GizmoSpace::World;
    GizmoLocation  preferred_location   = GizmoLocation::AABBMidpoint;
    bool           display_debug_window = false;

    void new_frame();
    void display(const mat4& view_mat, const mat4& proj_mat);
};


} // namespace josh
