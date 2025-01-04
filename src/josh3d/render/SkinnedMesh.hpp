#pragma once
#include "MeshStorage.hpp"
#include "VertexSkinned.hpp"
#include "Skeleton.hpp"
#include <memory>
#include <vector>


namespace josh {


/*
SkinnedMesh is simply a reference to a mesh in the storage plus a skeleton.

This is a rendering component.

TODO: Encapsulate better.
*/
struct SkinnedMesh {
    MeshID<VertexSkinned>           mesh_id;
    std::shared_ptr<const Skeleton> skeleton;
    std::vector<mat4>               skinning_mats; // Per-joint B2J-equivalent active transformations in mesh space.
};


} // namespace josh
