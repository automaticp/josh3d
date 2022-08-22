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
};




} // namespace learn
