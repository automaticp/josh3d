#pragma once
#include <entt/entity/entity.hpp>
#include <utility>
#include <vector>


namespace josh::components {


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
class Model {
private:
    std::vector<entt::entity> meshes_;

public:
    explicit Model(std::vector<entt::entity> meshes)
        : meshes_{ std::move(meshes) }
    {}

    const std::vector<entt::entity>& meshes() const noexcept { return meshes_; }
};


} // namespace josh::components
