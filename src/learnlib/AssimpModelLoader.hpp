#pragma once
#include <vector>
#include <span>
#include <string>
#include <utility>
#include <cstddef>
#include <memory>
#include <cassert>
#include <glbinding/gl/gl.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "GLObjects.hpp"
#include "GLObjectPool.hpp"
#include "Globals.hpp"
#include "Vertex.hpp"
#include "Model.hpp"
#include "Logging.hpp"


namespace learn {


// Provide specialization for your own Vertex layout
template<typename V>
std::vector<V> get_vertex_data(const aiMesh* mesh);



template<typename V = Vertex>
class AssimpModelLoader {
public:
    using flags_t = unsigned int;

private:
    std::vector<Mesh<V>> meshes_;

    std::string directory_;
    flags_t flags_;

    Assimp::Importer& importer_;

public:
    explicit AssimpModelLoader(
        flags_t flags =
            aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_ImproveCacheLocality
    ) : importer_{ default_importer() }, flags_{ flags } {}

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
            global_logstream << "[Assimp Error] " << importer_.GetErrorString() << '\n';
            // FIXME: Throw maybe?
        }

        assert(scene->mNumMeshes);

        meshes_.reserve(scene->mNumMeshes);
        process_node(scene->mRootNode, scene, meshes_);

        return *this;
    }

    [[nodiscard]]
    Model<V> get() {
        return Model<V>{ std::move(meshes_) };
    }

private:
    static Assimp::Importer& default_importer() {
        thread_local Assimp::Importer instance{};
        // FIXME: should be a way to free the resources in the importer
        // before the end of the runtime. Otherwise there's always some
        // zombie data hanging around.
        // NOTE: call FreeScene().
        return instance;
    }

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
        return globals::texture_handle_pool.load(full_path);
    }


    static std::vector<gl::GLuint> get_element_data(const aiMesh* mesh) {
        using namespace gl;
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





} // namespace learn
