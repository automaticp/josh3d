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
#include "MaterialDS.hpp"
#include "MeshData.hpp"
#include "DrawableMesh.hpp"
#include "Vertex.hpp"
#include "Model.hpp"
#include "Logging.hpp"


namespace learn {




inline std::vector<gl::GLuint> get_element_data(const aiMesh* mesh) {
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

} // namespace error





// Base component that deals with flags
// and other common things.
template<typename CRTP>
class AssimpLoaderBase {
public:
    using ai_flags_t = unsigned int;

protected:
    Assimp::Importer& importer_{ default_importer() };

    static Assimp::Importer& default_importer() {
        // This is per-vertex specialization
        // so maybe ughhh...
        thread_local Assimp::Importer instance{};
        return instance;
    }

    static constexpr ai_flags_t default_flags =
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph;

    ai_flags_t flags_{ default_flags };

    AssimpLoaderBase() = default;
    explicit AssimpLoaderBase(Assimp::Importer& importer) : importer_{ importer } {}

public:
    void free_imported_scene() { importer_.FreeScene(); }

    CRTP& add_flags(ai_flags_t flags) {
        flags_ |= flags;
        return static_cast<CRTP&>(*this);
    }

    CRTP& remove_flags(ai_flags_t flags) {
        flags_ &= ~flags;
        return static_cast<CRTP&>(*this);
    }

    CRTP& reset_flags() {
        flags_ &= 0;
        return static_cast<CRTP&>(*this);
    }

    CRTP& reset_flags_to_default() {
        flags_ = default_flags;
        return static_cast<CRTP&>(*this);
    }

    ai_flags_t get_flags() const noexcept { return flags_; }

};



// Simple loader that aggregates mesh data
// and skips materials.
template<typename V = Vertex>
class AssimpMeshDataLoader : public AssimpLoaderBase<AssimpMeshDataLoader<V>> {
private:
    using Base = AssimpLoaderBase<AssimpMeshDataLoader<V>>;
    using Base::importer_;
    using Base::flags_;

    std::vector<MeshData<V>> mesh_data_;
    const aiScene* scene_;
    std::string path_;

public:
    using Base::Base;

    [[nodiscard]]
    std::vector<MeshData<V>> get() {
        return std::move(mesh_data_);
    }

    AssimpMeshDataLoader& load(const std::string& path) {

        const aiScene* new_scene{ importer_.ReadFile(path, flags_) };

        if (!new_scene) {
            global_logstream << "[Assimp Error] " << importer_.GetErrorString() << '\n';
            throw error::AssimpLoaderIOError(importer_.GetErrorString());
        }

        scene_ = new_scene;
        path_ = path;

        // assert(scene_->mNumMeshes);

        mesh_data_.reserve(scene_->mNumMeshes);
        process_node(scene_->mRootNode);

        return *this;
    }

private:
    void process_node(aiNode* node) {
        for (auto mesh_id : std::span(node->mMeshes, node->mNumMeshes)) {
            aiMesh* mesh{ scene_->mMeshes[mesh_id] };
            mesh_data_.emplace_back(
                get_vertex_data<V>(mesh), get_element_data(mesh)
            );
        }

        for (aiNode* child : std::span(node->mChildren, node->mNumChildren)) {
            process_node(child);
        }
    }

};



template<typename V = Vertex>
class AssimpModelLoader : public AssimpLoaderBase<AssimpModelLoader<V>> {
private:
    using Base = AssimpLoaderBase<AssimpModelLoader<V>>;
    using Base::importer_;
    using Base::flags_;

    // Per-load state.
    std::vector<DrawableMesh> meshes_;
    std::vector<MeshData<V>> mesh_data_;
    const aiScene* scene_;
    std::string path_;
    std::string directory_;

public:
    using Base::Base;

    [[nodiscard]]
    Model get() {
        return Model{ std::move(meshes_) };
    }

    AssimpModelLoader& load(const std::string& path) {

        const aiScene* new_scene{ importer_.ReadFile(path, flags_) };

        if (!new_scene) {
            global_logstream << "[Assimp Error] " << importer_.GetErrorString() << '\n';
            throw error::AssimpLoaderIOError(importer_.GetErrorString());
        }

        scene_ = new_scene;
        path_ = path;
        directory_ = path.substr(0ull, path.find_last_of('/') + 1);

        // THIS IS I/O, WHAT GODDAMN ASSERTS ARE WE TALKING ABOUT???
        // assert(scene_->mNumMeshes);

        meshes_.reserve(scene_->mNumMeshes);
        mesh_data_.reserve(scene_->mNumMeshes);
        process_node(scene_->mRootNode);

        return *this;
    }


private:
    void process_node(aiNode* node) {

        for (auto mesh_id : std::span(node->mMeshes, node->mNumMeshes)) {
            aiMesh* mesh{ scene_->mMeshes[mesh_id] };
            mesh_data_.emplace_back(
                get_vertex_data<V>(mesh), get_element_data(mesh)
            );
            meshes_.emplace_back(mesh_data_.back(), get_material(mesh));
        }

        for (auto child : std::span(node->mChildren, node->mNumChildren)) {
            process_node(child);
        }
    }

    // Could be a template of MaterialT with the user providing an implementation.
    MaterialDS get_material(const aiMesh* mesh) {

        aiMaterial* material{ scene_->mMaterials[mesh->mMaterialIndex] };

        Shared<TextureHandle> diffuse =
            get_texture_from_material(material, aiTextureType_DIFFUSE);

        Shared<TextureHandle> specular =
            get_texture_from_material(material, aiTextureType_SPECULAR);

        if (!diffuse) { diffuse = globals::default_diffuse_texture; }
        if (!specular) { specular = globals::default_specular_texture; }

        return MaterialDS{
            .textures = { std::move(diffuse), std::move(specular) },
            .shininess = 128.f
        };

    }

    Shared<TextureHandle> get_texture_from_material(
        const aiMaterial* material, aiTextureType type)
    {

        if (material->GetTextureCount(type) == 0) {
            return nullptr;
        }

        aiString filename;
        material->GetTexture(type, 0ull, &filename);

        std::string full_path{ directory_ + filename.C_Str() };

        // FIXME: pool should be a c-tor parameter of something
        return globals::texture_handle_pool.load(full_path);
    }

};





} // namespace learn
