#pragma once
#include "GLScalars.hpp"
#include <assimp/mesh.h>
#include <vector>
#include <span>


namespace josh {


inline std::vector<GLuint> get_element_data(const aiMesh* mesh) {
    std::vector<GLuint> indices;
    indices.reserve(mesh->mNumFaces * 3ull);

    for (const auto& face : std::span<aiFace>(mesh->mFaces, mesh->mNumFaces)) {
        for (auto index : std::span<GLuint>(face.mIndices, face.mNumIndices)) {
            indices.emplace_back(index);
        }
    }
    return indices;
}


// Provide specialization for your own Vertex layout.
template<typename VertexT>
std::vector<VertexT> get_vertex_data(const aiMesh* mesh);


// Provide specialization for you own Material type.
template<typename MaterialT>
MaterialT get_material(const struct ModelLoadingContext& context,
    const aiMesh* mesh);



} // namespace josh
