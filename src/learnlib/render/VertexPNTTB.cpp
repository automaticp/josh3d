#include "VertexPNTTB.hpp"
#include "AssimpModelLoader.hpp"
#include <assimp/vector3.h>
#include <glbinding/gl/boolean.h>
#include <glbinding/gl/gl.h>
#include <vector>

namespace learn {



template<>
std::vector<VertexPNTTB> get_vertex_data<VertexPNTTB>(const aiMesh* mesh) {

    std::span<aiVector3D> positions(mesh->mVertices, mesh->mNumVertices);
    std::span<aiVector3D> normals(mesh->mNormals, mesh->mNumVertices);
    std::span<aiVector3D> tex_uvs(mesh->mTextureCoords[0], mesh->mNumVertices);
    std::span<aiVector3D> tangents(mesh->mTangents, mesh->mNumVertices);
    std::span<aiVector3D> bitangents(mesh->mBitangents, mesh->mNumVertices);

    if (!mesh->mNormals) {
        throw error::AssimpLoaderSceneParseError(
            "Mesh data does not contain Normals");
    }

    if (!mesh->mTextureCoords[0]) {
        throw error::AssimpLoaderSceneParseError(
            "Mesh data does not contain Texture Coordinates");
    }

    if (!mesh->mTangents) {
        throw error::AssimpLoaderSceneParseError(
            "Mesh data does not contain Tangents");
    }

    if (!mesh->mBitangents) {
        throw error::AssimpLoaderSceneParseError(
            "Mesh data does not contain Bitangents");
    }


    std::vector<VertexPNTTB> vertices;
    vertices.reserve(mesh->mNumVertices);
    for (size_t i{ 0 }; i < mesh->mNumVertices; ++i) {
        vertices.emplace_back(
            VertexPNTTB{
                .position= { positions[i].x,  positions[i].y,  positions[i].z },
                .normal=   { normals[i].x,    normals[i].y,    normals[i].z },
                .tex_uv=   { tex_uvs[i].x,    tex_uvs[i].y },
                .tangent=  { tangents[i].x,   tangents[i].y,   tangents[i].z },
                .bitangent={ bitangents[i].x, bitangents[i].y, bitangents[i].z }
            }
        );
    }

    return vertices;
}







} // namespace learn
