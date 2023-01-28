#include "Vertex.hpp"
#include "AssimpModelLoader.hpp"


namespace learn {



template<>
std::vector<Vertex> get_vertex_data<Vertex>(const aiMesh* mesh) {
    std::span<aiVector3D> positions(mesh->mVertices, mesh->mNumVertices);
    std::span<aiVector3D> normals(mesh->mNormals, mesh->mNumVertices);
    // Why texture coordinates are in a 3D space?
    std::span<aiVector3D> tex_uvs(mesh->mTextureCoords[0], mesh->mNumVertices);

    if (!mesh->mNormals) {
        throw error::AssimpLoaderSceneParseError(
            "Mesh data does not contain Normals");
    }

    if (!mesh->mTextureCoords[0]) {
        throw error::AssimpLoaderSceneParseError(
            "Mesh data does not contain Texture Coordinates");
    }

    std::vector<Vertex> vertices;
    vertices.reserve(mesh->mNumVertices);
    for (size_t i{ 0 }; i < mesh->mNumVertices; ++i) {
        vertices.emplace_back(
            Vertex{
                glm::vec3{ positions[i].x,  positions[i].y,  positions[i].z },
                glm::vec3{ normals[i].x,    normals[i].y,    normals[i].z   },
                glm::vec2{ tex_uvs[i].x,    tex_uvs[i].y    }
            }
        );
    }

    return vertices;
}


} // namespace learn
