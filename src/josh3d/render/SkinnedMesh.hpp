#pragma once
#include "LODPack.hpp"
#include "MeshStorage.hpp"
#include "Resource.hpp"
#include "VertexSkinned.hpp"
#include "Skeleton.hpp"
#include <cstdint>
#include <memory>
#include <ranges>
#include <vector>


namespace josh {


/*
TODO: Encapsulate.
*/
struct PosedSkeleton {
    PosedSkeleton(std::shared_ptr<const Skeleton> skeleton)
        : skeleton{ MOVE(skeleton) }
        , M2Js{
            this->skeleton->joints |
            std::views::transform([](const Joint& j) { return inverse(j.inv_bind); }) |
            ranges::to<std::vector>()
        }
        , skinning_mats(this->skeleton->joints.size(), glm::identity<mat4>())
    {}

    std::shared_ptr<const Skeleton> skeleton;
    std::vector<mat4>               M2Js;          // Mesh->Joint CoB matrices. It is convenient to store this.
    std::vector<mat4>               skinning_mats; // Per-joint B2J-equivalent active transformations in mesh space.
};


/*
SkinnedMesh is simply a reference to a mesh in the storage plus a posed skeleton.

This is a rendering component.

TODO: Encapsulate better.
*/
struct SkinnedMesh {
    MeshID<VertexSkinned> mesh_id;
    PosedSkeleton         pose;
};


struct SkinnedMe2h {
    LODPack<MeshID<VertexSkinned>, 8> lods;
    ResourceUsage                     usage;
    // TODO: Shouldn't PublicResource be a vocabulary type?
    std::shared_ptr<const Skeleton>   skeleton;
    ResourceUsage                     skeleton_usage;
    uintptr_t                         aba_tag{};
};


} // namespace josh
