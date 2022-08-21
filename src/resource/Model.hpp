#pragma once
#include <vector>
#include <utility>
#include <span>
#include <memory>
#include <cstddef>
#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Mesh.hpp"
#include "GLObjects.hpp"
#include "GLObjectPools.hpp"
#include "ShaderProgram.h"

namespace learn {

using namespace gl;




template<typename V>
class AssimpModelLoader;


template<typename V>
class Model {
private:
    std::vector<Mesh<V>> meshes_;

public:
    explicit Model(std::vector<Mesh<V>> meshes) : meshes_{ std::move(meshes) } {}

    void draw(ShaderProgram& sp) {
        for (auto&& mesh : meshes_) {
            mesh.draw(sp);
        }
    }

private:
    friend class AssimpModelLoader<V>;

    Model() = default;
};


// Provide specialization for your own Vertex layout
template<typename V>
std::vector<V> get_vertex_data(const aiMesh* mesh);

template<typename V>
class AssimpModelLoader {
public:
    using flags_t = unsigned int;

private:
    Model<V> model_;

    std::string directory_;
    flags_t flags_;

    Assimp::Importer& importer_;

public:
    explicit AssimpModelLoader(
        Assimp::Importer& importer,
        flags_t flags =
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_ImproveCacheLocality
    ) : importer_{ importer }, flags_{ flags } {}

    AssimpModelLoader& add_flags(flags_t flags) {
        flags_ |= flags;
        return *this;
    }

    AssimpModelLoader& remove_flags(flags_t flags) {
        flags_ &= ~flags;
        return *this;
    }

    AssimpModelLoader& reset_flags() {
        flags_ &= 0;
        return *this;
    }

    AssimpModelLoader& load(const std::string& path) {

        directory_ = path.substr(0ull, path.find_last_of('/') + 1);

        const aiScene* scene{ importer_.ReadFile(path, flags_) };

        if ( std::strlen(importer_.GetErrorString()) != 0 ) {
            std::cerr << "[Assimp Error] " << importer_.GetErrorString() << '\n';
            // FIXME: Throw maybe?
        }

        model_ = Model<V>();
        model_.meshes_.reserve(scene->mNumMeshes);
        process_node(scene->mRootNode, scene, model_.meshes_);

        return *this;
    }

    [[nodiscard]]
    Model<V>&& get() {
        return std::move(model_);
    }

private:
    void process_node(aiNode* node, const aiScene* scene, std::vector<Mesh<V>>& meshes) {

        for ( auto&& mesh_id : std::span(node->mMeshes, node->mNumMeshes) ) {
            aiMesh* mesh{ scene->mMeshes[mesh_id] };
            meshes.emplace_back(get_mesh_data(mesh, scene));
        }

        for ( auto&& child : std::span(node->mChildren, node->mNumChildren) ) {
            process_node(child, scene, meshes);
        }
    }


    Mesh<V> get_mesh_data(const aiMesh* mesh, const aiScene* scene) {
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material{ scene->mMaterials[mesh->mMaterialIndex] };
            auto diffuse  = get_texture_from_material(material, aiTextureType_DIFFUSE);
            auto specular = get_texture_from_material(material, aiTextureType_SPECULAR);
            return Mesh<V>(
                get_vertex_data<V>(mesh), get_element_data(mesh), std::move(diffuse), std::move(specular)
            );
        } else {
            throw std::runtime_error("The requested mesh has no valid material index");
        }
    }


    std::shared_ptr<TextureHandle>
    get_texture_from_material(const aiMaterial* material, aiTextureType type) {

        assert(material->GetTextureCount(type) == 1);

        aiString filename;
        material->GetTexture(type, 0ull, &filename);

        std::string full_path{ directory_ + filename.C_Str() };

        // FIXME: pool should be a c-tor parameter of something
        return default_texture_handle_pool.load(full_path);
    }


    static std::vector<GLuint> get_element_data(const aiMesh* mesh) {
        std::vector<GLuint> indices;
        // FIXME: Is there a way to reserve correctly?
        indices.reserve(mesh->mNumFaces * 3ull);
        for ( const auto& face : std::span<aiFace>(mesh->mFaces, mesh->mNumFaces) ) {
            for ( const auto& index : std::span<GLuint>(face.mIndices, face.mNumIndices) ) {
                indices.emplace_back(index);
            }
        }
        return indices;
    }
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
