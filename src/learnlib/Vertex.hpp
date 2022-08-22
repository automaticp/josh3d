#pragma once
#include <array>
#include <vector>
#include <span>
#include <cstddef>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>

#include "VertexTraits.hpp"
#include "AssimpModelLoader.hpp"

namespace learn {



struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_uv;
};


template<>
struct VertexTraits<Vertex> {
    constexpr static std::array<AttributeParams, 3> aparams{{
        { 0u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex), offsetof(Vertex, position) },
        { 1u, 3, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal) },
        { 2u, 2, gl::GL_FLOAT, gl::GL_FALSE, sizeof(Vertex), offsetof(Vertex, tex_uv) }
    }};
};


template<>
inline std::vector<Vertex> get_vertex_data(const aiMesh* mesh) {
    std::span<aiVector3D> positions( mesh->mVertices, mesh->mNumVertices );
    std::span<aiVector3D> normals( mesh->mNormals, mesh->mNumVertices );
    // Why texture coordinates are in a 3D space?
    std::span<aiVector3D> tex_uvs( mesh->mTextureCoords[0], mesh->mNumVertices );

    if ( !mesh->mNormals || !mesh->mTextureCoords[0] ) {
        assert(false);
        // FIXME: Throw maybe?
        // Violates assumptions about the contents of Vertex.
    }

    std::vector<Vertex> vertices;
    vertices.reserve(mesh->mNumVertices);
    for ( size_t i{ 0 }; i < mesh->mNumVertices; ++i ) {
        vertices.emplace_back(
            glm::vec3{ positions[i].x,  positions[i].y,  positions[i].z },
            glm::vec3{ normals[i].x,    normals[i].y,    normals[i].z   },
            glm::vec2{ tex_uvs[i].x,    tex_uvs[i].y    }
        );
    }

    return vertices;
}




} // namespace learn
