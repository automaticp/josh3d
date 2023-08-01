#pragma once
#include "AssimpLoaderTemplates.hpp"
#include "Filesystem.hpp"
#include "GLObjects.hpp"
#include "MeshData.hpp"
#include "Model.hpp"
#include "RuntimeError.hpp"
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
#include <string>
#include <utility>
#include <vector>


namespace josh {


namespace error {

// TODO: Assimp has its own exceptions, I think, so look into that maybe.

class AssimpLoaderError : public RuntimeError {
public:
    static constexpr auto prefix = "Assimp Loader Error: ";
    AssimpLoaderError(std::string msg)
        : AssimpLoaderError(prefix, std::move(msg))
    {}
protected:
    AssimpLoaderError(const char* prefix, std::string msg)
        : RuntimeError(prefix, std::move(msg))
    {}
};




// TODO: Can this be classified more accurately?
// Exact reasons why read fails? Do I need to?
class AssimpLoaderReadFileFailure : public AssimpLoaderError {
public:
    static constexpr auto prefix = "Assimp Loader File Reading Failure: ";
    Path path;
    AssimpLoaderReadFileFailure(Path path, std::string error_string)
        : AssimpLoaderReadFileFailure(prefix,
            std::move(path), std::move(error_string))
    {}
protected:
    AssimpLoaderReadFileFailure(const char* prefix,
        Path path, std::string error_string)
        : AssimpLoaderError(prefix, std::move(error_string))
        , path{ std::move(path) }
    {}
};


class AssimpLoaderSceneParseError : public AssimpLoaderError {
public:
    static constexpr auto prefix = "Assimp Loader Scene Parsing Error: ";
    AssimpLoaderSceneParseError(std::string msg)
        : AssimpLoaderSceneParseError(prefix, std::move(msg))
    {}
protected:
    AssimpLoaderSceneParseError(const char* prefix, std::string msg)
        : AssimpLoaderError(prefix, std::move(msg))
    {}
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

public:
    using Base::Base;

    [[nodiscard]]
    std::vector<MeshData<V>> get() {
        return std::move(mesh_data_);
    }

    AssimpMeshDataLoader& load(const File& file) {

        const aiScene* new_scene{ importer_.ReadFile(file.path(), flags_) };

        if (!new_scene) {
            throw error::AssimpLoaderReadFileFailure{ file.path(), importer_.GetErrorString() };
        }

        scene_ = new_scene;

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
    File file;
    Directory directory;
};




class ModelComponentLoader
    : public detail::AssimpLoaderBase<ModelComponentLoader>
{
private:
    using Base = AssimpLoaderBase<ModelComponentLoader>;
    using Base::importer_;
    using Base::flags_;

    struct Output {
        std::vector<entt::entity> meshes;
    };

public:
    using Base::Base;

    ModelComponent& load_into(entt::handle model_handle, const File& file)
    {
        // FIXME: Who specifies this?
        add_flags(aiProcess_CalcTangentSpace);

        const aiScene* new_scene{ importer_.ReadFile(file.path(), flags_) };

        if (!new_scene) {
            throw error::AssimpLoaderReadFileFailure{ file.path(), importer_.GetErrorString() };
        }

        ModelLoadingContext context{
            .scene = new_scene,
            .file  = file,
            .directory = Directory{ file.path().parent_path() }
        };

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
