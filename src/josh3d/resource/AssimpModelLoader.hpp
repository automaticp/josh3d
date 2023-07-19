#pragma once
#include "AssimpLoaderTemplates.hpp"
#include "GLObjects.hpp"
#include "GlobalsUtil.hpp"
#include "MeshData.hpp"
#include "Model.hpp"
#include "VertexPNT.hpp"
#include <assimp/Exceptional.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glbinding/gl/gl.h>
#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>


namespace josh {


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




namespace detail {



/*
Base implementation component that deals with flags
and other common things.
*/
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
        aiProcess_Triangulate | aiProcess_ImproveCacheLocality |
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


} // namespace detail





/*
Simple loader that aggregates mesh data
and skips materials.

TODO: Still used in PointLightBoxStage but should
be deprecated otherwise.
*/
template<typename V = VertexPNT>
class AssimpMeshDataLoader
    : public detail::AssimpLoaderBase<AssimpMeshDataLoader<V>>
{
private:
    using Base = detail::AssimpLoaderBase<AssimpMeshDataLoader<V>>;
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
            globals::logstream << "[Assimp Error] " << importer_.GetErrorString() << '\n';
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








struct ModelLoadingContext {
    const aiScene* scene{};
    std::string path;
    std::string directory;
};




class ModelComponentLoader2
    : public detail::AssimpLoaderBase<ModelComponentLoader2>
{
private:
    using Base = AssimpLoaderBase<ModelComponentLoader2>;
    using Base::importer_;
    using Base::flags_;

    struct Output {
        std::vector<entt::entity> meshes;
    };

public:
    using Base::Base;

    ModelComponent& load_into(entt::handle model_handle, const char* path)
    {
        // FIXME: Who specifies this?
        add_flags(aiProcess_CalcTangentSpace);

        const aiScene* new_scene{ importer_.ReadFile(path, flags_) };

        if (!new_scene) {
            throw error::AssimpLoaderIOError(importer_.GetErrorString());
        }

        ModelLoadingContext context{
            .scene=new_scene
        };

        context.path = path;
        context.directory =
            context.path.substr(0ull, context.path.find_last_of('/') + 1);


        std::vector<entt::entity> output_meshes;
        output_meshes.reserve(context.scene->mNumMeshes);

        for (aiMesh* mesh : std::span(context.scene->mMeshes, context.scene->mNumMeshes)) {
            emplace_mesh(output_meshes, mesh, model_handle, context);
        }

        return model_handle.emplace<ModelComponent>(std::move(output_meshes));
    }

private:
    void emplace_mesh(std::vector<entt::entity>& output_meshes,
        aiMesh* mesh, entt::handle model_handle,
        const ModelLoadingContext& context);

    void emplace_material_components(entt::handle mesh_handle,
        aiMaterial* material, const ModelLoadingContext& context);


};






} // namespace josh
