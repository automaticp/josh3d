#pragma once
#include "GLObjects.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <glbinding/gl/types.h>
#include <cassert>
#include <utility>
#include <vector>



namespace learn {


/*
Mesh entity:

Mesh
Transform
Material (optional)
ChildMesh (optional)

Model entity:

set<Mesh>
Transform

*/
class ModelComponent {
private:
    std::vector<entt::entity> meshes_;

public:
    explicit ModelComponent(std::vector<entt::entity> meshes)
        : meshes_{ std::move(meshes) }
    {}

    const std::vector<entt::entity>& meshes() const noexcept { return meshes_; }
};


struct ChildMesh {
    entt::entity parent;

    ChildMesh(entt::entity parent_entity)
        : parent{ parent_entity }
    {
        assert(parent != entt::null);
    }
};



} // namespace learn
