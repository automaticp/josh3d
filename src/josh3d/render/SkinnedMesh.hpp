#pragma once
#include "MeshStorage.hpp"
#include "VertexSkinned.hpp"
#include "Skeleton.hpp"
#include <memory>


namespace josh {


/*
SkinnedMesh is simply a reference to a mesh in the storage plus a skeleton.

This is a rendering component.

TODO: Encapsulate better.
*/
struct SkinnedMesh {
    MeshID<VertexSkinned>           mesh_id;
    std::shared_ptr<const Skeleton> skeleton;
    // TODO: Pose?
};


} // namespace josh
