#pragma once
#include <vector>
#include <utility>

#include "Vertex.hpp"
#include "Mesh.hpp"
#include "GLObjects.hpp"




namespace learn {



template<typename V = Vertex>
class Model {
private:
    std::vector<Mesh<V>> meshes_;

public:
    explicit Model(std::vector<Mesh<V>> meshes) : meshes_{ std::move(meshes) } {}

    void draw(ActiveShaderProgram& sp) {
        for (auto&& mesh : meshes_) {
            mesh.draw(sp);
        }
    }

    void draw(ActiveShaderProgram& sp, gl::GLint diffuse_loc, gl::GLint specular_loc, gl::GLint shininess_loc) {
        for (auto&& mesh : meshes_) {
            mesh.draw(sp, diffuse_loc, specular_loc, shininess_loc);
        }
    }

    const std::vector<Mesh<V>>& mehses() const noexcept {
        return meshes_;
    }

    std::vector<Mesh<V>>& mehses() noexcept {
        return meshes_;
    }
};




} // namespace learn
