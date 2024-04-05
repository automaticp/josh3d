#include "AssimpModelLoader.hpp"
#include "AttributeTraits.hpp"
#include "GLTextures.hpp"
#include "components/ChildMesh.hpp"
#include "components/BoundingSphere.hpp"
#include "components/Mesh.hpp"
#include "components/Name.hpp"
#include "components/Materials.hpp"
#include "tags/AlphaTested.hpp"
#include "Transform.hpp"
#include "VertexPNTTB.hpp"
#include "TexturePools.hpp"
#include <entt/entt.hpp>
#include <algorithm>


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

    mesh_handle.emplace<components::Mesh>(mesh_data);
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
                return { TextureType::Diffuse };
            case aiTextureType_SPECULAR:
                return { TextureType::Specular };
            case aiTextureType_NORMALS:
                return { TextureType::Normal };
            case aiTextureType_HEIGHT:
                return { TextureType::Normal };
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
                case Diffuse: {
                    auto& diffuse =
                        mesh_handle.emplace<components::MaterialDiffuse>(std::move(texture));
                    // TODO: We check if the alpha channel even exitsts in the texture,
                    // to decide on whether alpha testing should be enabled. Is there a better way?
                    PixelComponentType alpha_component =
                        diffuse.texture->get_component_type<PixelComponent::Alpha>();
                    if (alpha_component != PixelComponentType::None) {
                        mesh_handle.emplace<tags::AlphaTested>();
                    }
                } break;
                case Specular: {
                    // FIXME: Shininess? Ah, whatever, we don't even store it in the gbuffer.
                    mesh_handle.emplace<components::MaterialSpecular>(std::move(texture), 128.f);
                } break;
                case Normal: {
                    mesh_handle.emplace<components::MaterialNormal>(std::move(texture));
                } break;
                default:
                    // TODO: Is this REALLY an assert condition?
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





