#pragma once
#include "GLObjects.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <glbinding/gl/types.h>
#include <cassert>
#include <utility>
#include <vector>



namespace josh {


/*
Mesh entity:

Mesh
Transform
Material (optional)
components::ChildMesh (optional)

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




} // namespace josh
