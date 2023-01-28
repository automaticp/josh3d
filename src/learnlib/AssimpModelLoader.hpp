#pragma once
#include <stdexcept>
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
#include <assimp/Exceptional.h>

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


namespace error {

// TODO: Assimp has its own exceptions, I think, so look into that maybe.

class AssimpLoaderError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class AssimpLoaderIOError : public AssimpLoaderError {
public:
    using AssimpLoaderError::AssimpLoaderError;
};

class AssimpLoaderSceneParseError : public AssimpLoaderError {
public:
    using AssimpLoaderError::AssimpLoaderError;
};

}



template<typename V = Vertex>
class AssimpModelLoader {
public:
    using ai_flags_t = unsigned int;

private:
    Assimp::Importer& importer_{ default_importer() };

    static constexpr ai_flags_t default_flags =
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph;

    ai_flags_t flags_{ default_flags };

    // Per-load state.
    std::vector<Mesh<V>> meshes_;
    const aiScene* scene_;
    std::string directory_;


    static Assimp::Importer& default_importer() {
        thread_local Assimp::Importer instance{};
        return instance;
    }

public:
    AssimpModelLoader() = default;


    void free_imported_scene() { importer_.FreeScene(); }

    AssimpModelLoader& add_flags(ai_flags_t flags) {
        flags_ |= flags;
        return *this;
    }

    AssimpModelLoader& remove_flags(ai_flags_t flags) {
        flags_ &= ~flags;
        return *this;
    }

    AssimpModelLoader& reset_flags() {
        flags_ &= 0;
        return *this;
    }

    AssimpModelLoader& reset_flags_to_default() {
        flags_ = default_flags;
        return *this;
    }

    [[nodiscard]]
    Model<V> get() {
        return Model<V>{ std::move(meshes_) };
    }


    AssimpModelLoader& load(const std::string& path) {

        const aiScene* new_scene{ importer_.ReadFile(path, flags_) };

        if (!new_scene) {
            global_logstream << "[Assimp Error] " << importer_.GetErrorString() << '\n';
            throw error::AssimpLoaderIOError(importer_.GetErrorString());
        }

        scene_ = new_scene;
        directory_ = path.substr(0ull, path.find_last_of('/') + 1);

        // THIS IS I/O, WHAT GODDAMN ASSERTS ARE WE TALKING ABOUT???
        assert(scene_->mNumMeshes);

        meshes_.reserve(scene_->mNumMeshes);
        process_node(scene_->mRootNode);

        return *this;
    }


private:
    void process_node(aiNode* node) {

        for (auto mesh_id : std::span(node->mMeshes, node->mNumMeshes)) {
            aiMesh* mesh{ scene_->mMeshes[mesh_id] };
            meshes_.emplace_back(get_mesh_data(mesh));
        }

        for (auto child : std::span(node->mChildren, node->mNumChildren)) {
            process_node(child);
        }
    }


    Mesh<V> get_mesh_data(const aiMesh* mesh) {

        aiMaterial* material{ scene_->mMaterials[mesh->mMaterialIndex] };

        Shared<TextureHandle> diffuse =
            get_texture_from_material(material, aiTextureType_DIFFUSE);

        Shared<TextureHandle> specular =
            get_texture_from_material(material, aiTextureType_SPECULAR);

        return Mesh<V>(
            get_vertex_data<V>(mesh), get_element_data(mesh), std::move(diffuse), std::move(specular)
        );
    }

    Shared<TextureHandle>
    get_texture_from_material(const aiMaterial* material, aiTextureType type) {

        if (material->GetTextureCount(type) == 0) {
            // FIXME: substitute with a default texture of some kind maybe.
            // Here or later, not sure. Probably later.
            return nullptr;
        }

        aiString filename;
        material->GetTexture(type, 0ull, &filename);

        std::string full_path{ directory_ + filename.C_Str() };

        // FIXME: pool should be a c-tor parameter of something
        return globals::texture_handle_pool.load(full_path);
    }


    static std::vector<gl::GLuint> get_element_data(const aiMesh* mesh) {
        using namespace gl;
        std::vector<GLuint> indices;
        indices.reserve(mesh->mNumFaces * 3ull);

        for (const auto& face : std::span<aiFace>(mesh->mFaces, mesh->mNumFaces)) {
            for (auto index : std::span<GLuint>(face.mIndices, face.mNumIndices)) {
                indices.emplace_back(index);
            }
        }
        return indices;
    }


};





} // namespace learn
