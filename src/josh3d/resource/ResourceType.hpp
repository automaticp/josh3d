#pragma once


namespace josh {


/*
NOTE: This list is technically extendable per-project,
as long as new types don't overlap with any of the existing values.

Odd, but should work.
*/
enum class ResourceType : uint32_t {
    Scene     = 0,
    Mesh      = 1,
    Texture   = 2,
    Animation = 3,
    Skeleton  = 4,
    Material  = 5, // TODO:
    MeshDesc  = 6, // Mesh description file containing mesh and material.
};


} // namespace josh
