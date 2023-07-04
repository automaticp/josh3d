#pragma once
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "GlobalsUtil.hpp"
#include "Mesh.hpp"
#include "MeshData.hpp"
#include "Model.hpp"
#include "RenderComponents.hpp"
#include "Transform.hpp"
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


namespace learn {




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




class ModelComponentLoader
    : public detail::AssimpLoaderBase<ModelComponentLoader>
{
private:
    using Base = AssimpLoaderBase<ModelComponentLoader>;
    using Base::importer_;
    using Base::flags_;

    template<typename VertexT, typename MaterialT>
    struct Output {
        std::vector<entt::entity> meshes;
    };

    struct ECSContext {
        entt::registry& registry;
        entt::entity model_entity;

        ECSContext(entt::registry& registry,
            entt::entity model_entity)
            : registry{ registry }
            , model_entity{ model_entity }
        {}
    };

public:
    using Base::Base;


    // For cases where VertexT and MaterialT are known.
    template<typename VertexT, typename MaterialT>
    ModelComponent& load_into(entt::registry& registry,
        entt::entity model_entity, const char* path)
    {
        const aiScene* new_scene{ importer_.ReadFile(path, flags_) };

        if (!new_scene) {
            throw error::AssimpLoaderIOError(importer_.GetErrorString());
        }


        Output<VertexT, MaterialT> output{};

        ModelLoadingContext context{
            .scene=new_scene
        };

        context.path = path;
        context.directory =
            context.path.substr(0ull, context.path.find_last_of('/') + 1);

        output.meshes.reserve(context.scene->mNumMeshes);

        ECSContext ecs_context{ registry, model_entity };

        process_node(
            output, ecs_context, context, context.scene->mRootNode
        );

        return registry.emplace<ModelComponent>(model_entity, std::move(output.meshes));
    }


private:
    template<typename VertexT, typename MaterialT>
    void process_node(
        Output<VertexT, MaterialT>& output,
        ECSContext& ecs_context,
        const ModelLoadingContext& context,
        aiNode* node)
    {

        for (auto mesh_id
            : std::span(node->mMeshes, node->mNumMeshes))
        {
            aiMesh* mesh = context.scene->mMeshes[mesh_id];

            MeshData mesh_data{
                get_vertex_data<VertexT>(mesh),
                get_element_data(mesh)
            };

            // TODO: Maybe cache mesh_data here.

            auto& r = ecs_context.registry;
            auto new_entity = r.create();

            r.emplace<Mesh>(new_entity, mesh_data);


            // Point of type erasure for MaterialT.
            r.emplace<MaterialT>(new_entity, get_material<MaterialT>(context, mesh));


            // Link ModelComponent and Mesh.
            r.emplace<components::ChildMesh>(new_entity, ecs_context.model_entity);
            output.meshes.emplace_back(new_entity);

            // FIXME: Transform Component?
            r.emplace<Transform>(new_entity);

            r.emplace<components::Name>(new_entity, mesh->mName.C_Str());

        }


        for (auto child
            : std::span(node->mChildren, node->mNumChildren))
        {
            process_node(output, ecs_context, context, child);
        }
    }




};




} // namespace learn
