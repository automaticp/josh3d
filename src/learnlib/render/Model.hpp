#pragma once
#include "DrawableMesh.hpp"
#include "GLObjects.hpp"

#include <glbinding/gl/types.h>
#include <utility>
#include <vector>



namespace learn {


// Model is a collection of DrawableMeshes;
// DrawableMesh is a pair of a Mesh and a Material;
// Mesh is vertex data on the GPU;
// Material is texture data on the GPU and material parameters.
class Model {
private:
    std::vector<DrawableMesh> meshes_;

public:
    explicit Model(std::vector<DrawableMesh> meshes) : meshes_{ std::move(meshes) } {}

    const auto& drawable_meshes() const noexcept { return meshes_; }
    auto& drawable_meshes() noexcept { return meshes_; }


    void draw(ActiveShaderProgram& asp) {
        for (DrawableMesh& drawable : meshes_) {
            drawable.draw(asp);
        }
    }

    void draw(ActiveShaderProgram& asp, const MaterialDS::locations_type& locations) {
        for (DrawableMesh& drawable : meshes_) {
            drawable.draw(asp, locations);
        }
    }

    void draw_instanced(ActiveShaderProgram& asp, gl::GLsizei count) {
        for (DrawableMesh& drawable : meshes_) {
            drawable.draw_instanced(asp, count);
        }
    }

    void draw_instanced(ActiveShaderProgram& asp,
        const MaterialDS::locations_type& locations, gl::GLsizei count)
    {
        for (DrawableMesh& drawable : meshes_) {
            drawable.draw_instanced(asp, locations, count);
        }
    }

};




} // namespace learn
