#include "AssimpModelLoader.hpp"
#include "RenderComponents.hpp"
#include "Mesh.hpp"
#include "RenderComponents.hpp"
#include "Transform.hpp"
#include "VertexPNTTB.hpp"
#include "TextureHandlePool.hpp"
#include "GlobalsGL.hpp"
#include "FrustumCuller.hpp"
#include <algorithm>
#include <entt/entt.hpp>
#include <functional>


namespace josh {


static float bounding_radius(const std::vector<VertexPNTTB>& verts) noexcept {
    if (verts.empty()) { return 0.f; }
    auto max_elem =
        std::max_element(verts.begin(), verts.end(),
            [](const VertexPNTTB& vert1, const VertexPNTTB& vert2) {
                return glm::length(vert1.position) < glm::length(vert2.position);
            }
        );
    return glm::length(max_elem->position);
}




void ModelComponentLoader::emplace_mesh(std::vector<entt::entity>& output_meshes,
    aiMesh* mesh, entt::handle model_handle,
    const ModelLoadingContext& context)
{
    MeshData mesh_data{
        // FIXME: This won't work if the tangents are not generated.
        get_vertex_data<VertexPNTTB>(mesh),
        get_element_data(mesh)
    };

    // TODO: Maybe cache mesh_data here.

    auto& r = *model_handle.registry();
    auto mesh_handle = entt::handle(r, r.create());

    mesh_handle.emplace<Mesh>(mesh_data);
    mesh_handle.emplace<components::BoundingSphere>(bounding_radius(mesh_data.vertices()));

    aiMaterial* material = context.scene->mMaterials[mesh->mMaterialIndex];
    emplace_material_components(mesh_handle, material, context);

    // Link ModelComponent and Mesh
    mesh_handle.emplace<components::ChildMesh>(model_handle.entity());
    output_meshes.emplace_back(mesh_handle.entity());

    // FIXME: Transform Component?
    mesh_handle.emplace<Transform>();
    mesh_handle.emplace<components::Name>(mesh->mName.C_Str());

}




void ModelComponentLoader::emplace_material_components(
    entt::handle mesh_handle, aiMaterial* material,
    const ModelLoadingContext& context)
{

    auto get_full_path = [&](aiTextureType type) -> File {
        aiString filename;
        material->GetTexture(type, 0u, &filename);
        return File{ context.directory.path() / filename.C_Str() };
    };


    auto match_context = [](aiTextureType type)
        -> TextureHandleLoadContext
    {
        switch (type) {
            case aiTextureType_DIFFUSE:
                return { TextureType::diffuse };
            case aiTextureType_SPECULAR:
                return { TextureType::specular };
            case aiTextureType_NORMALS:
                return { TextureType::normal };
            case aiTextureType_HEIGHT:
                return { TextureType::normal };
            default:
                return {};
        }
    };


    auto load_and_emplace_if_exists = [&](aiTextureType type)
        -> bool
    {
        const bool texture_exists =
            material->GetTextureCount(type);

        if (texture_exists) {
            auto full_path = get_full_path(type);
            auto load_context = match_context(type);

            auto texture = globals::texture_handle_pool.load(
                full_path, load_context
            );

            switch (load_context.type) {
                using enum TextureType;
                case diffuse:
                    mesh_handle.emplace<components::MaterialDiffuse>(std::move(texture));
                    // FIXME: Alpha masking is enabled by default for all meshes
                    // with diffuse textures as we have no way to tell that from
                    // the loading contex yet.
                    mesh_handle.emplace<components::AlphaTested>();
                    break;
                case specular:
                    // FIXME: Shininess?
                    mesh_handle.emplace<components::MaterialSpecular>(std::move(texture), 128.f);
                    break;
                case normal:
                    mesh_handle.emplace<components::MaterialNormal>(std::move(texture));
                    break;
                default:
                    assert(false &&
                        "Requested unsupported TextureType");
            }
        }

        return texture_exists;
    };


    load_and_emplace_if_exists(aiTextureType_DIFFUSE);
    load_and_emplace_if_exists(aiTextureType_SPECULAR);
    if (!load_and_emplace_if_exists(aiTextureType_NORMALS)) {
        load_and_emplace_if_exists(aiTextureType_HEIGHT);
    }

}





} // namespace josh





