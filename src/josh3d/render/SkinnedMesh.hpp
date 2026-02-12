#pragma once
#include "Common.hpp"
#include "LODPack.hpp"
#include "MeshStorage.hpp"
#include "Resource.hpp"
#include "Scalars.hpp"
#include "SkeletonStorage.hpp"
#include "VertexFormats.hpp"
#include "Skeleton.hpp"
#include <memory>
#include <vector>


namespace josh {


using SkinnedMeshID = MeshID<VertexSkinned>;

/*
TODO: Encapsulate. TODO: Forget it.
*/
struct PosedSkeleton
{
    PosedSkeleton(std::shared_ptr<const Skeleton> skeleton);

    std::shared_ptr<const Skeleton> skeleton;
    std::vector<mat4>               M2Js;          // Mesh->Joint CoB matrices. It is convenient to store this.
    std::vector<mat4>               skinning_mats; // Per-joint B2J-equivalent active transformations in mesh space.
};

/*
SkinnedMesh is simply a reference to a mesh in the storage plus a posed skeleton.

This is a rendering component.

TODO: DEPRECATE
*/
struct SkinnedMesh
{
    SkinnedMeshID mesh_id;
    PosedSkeleton pose;
};


struct Pose
{
    Vector<mat4> M2Js;          // Mesh->Joint CoB matrices. It is convenient to store this.
    Vector<mat4> skinning_mats; // Per-joint B2J-equivalent active transformations in mesh space.

    static auto from_skeleton(const Skeleton& skeleton)
        -> Pose;
};

struct SkinnedMe2h
{
    LODPack<SkinnedMeshID, 8> lods;
    ResourceUsage             usage;
    // TODO: Shouldn't PublicResource be a vocabulary type?
    std::shared_ptr<const Skeleton> skeleton;
    ResourceUsage             skeleton_usage;
    uintptr                   aba_tag = {};
};

/*
Version 3 compatible with SkeletonStorage.
*/
struct SkinnedMe3h
{
    LODPack<SkinnedMeshID, 8> lods;
    ResourceUsage        usage;
    SkeletonID           skeleton_id;
    ResourceUsage        skeleton_usage;
    uintptr              aba_tag = {};
};

} // namespace josh
