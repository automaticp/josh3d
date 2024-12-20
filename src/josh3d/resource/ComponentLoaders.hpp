#pragma once
#include "Asset.hpp"
#include "CubemapData.hpp"
#include "ECS.hpp"
#include "Filesystem.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "Pixels.hpp"
#include "SceneGraph.hpp"
#include "TextureHelpers.hpp"
#include "Transform.hpp"
#include "Materials.hpp"
#include "Mesh.hpp"
#include "Name.hpp"
#include "Skybox.hpp"
#include "tags/AlphaTested.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>


namespace josh {


inline Skybox& load_skybox_into(
    Handle      handle,
    const File& skybox_json)
{
    CubemapPixelData<pixel::RGBA> data =
        load_cubemap_pixel_data_from_json<pixel::RGBA>(skybox_json);

    UniqueCubemap cubemap =
        create_skybox_from_cubemap_pixel_data(data, InternalFormat::SRGBA8);

    Skybox& skybox =
        handle.emplace_or_replace<Skybox>(std::move(cubemap));

    return skybox;
}




inline void emplace_model_asset_into(
    Handle           model_handle,
    SharedModelAsset asset)
{
    auto& registry = *model_handle.registry();

    thread_local std::vector<entt::entity> children;
    children.resize(asset.meshes.size());
    registry.create(children.begin(), children.end());

    try {

        for (size_t i{ 0 }; i < children.size(); ++i) {

            SharedMeshAsset& mesh = asset.meshes[i];
            const Handle mesh_handle{ registry, children[i] };

            // Bind to make assets available in this thread.
            make_available<Binding::ArrayBuffer>       (mesh.vertices->id());
            make_available<Binding::ElementArrayBuffer>(mesh.indices->id() );

            mesh_handle.emplace<Mesh>(
                Mesh::from_buffers<VertexPNUTB>(
                    std::move(mesh.vertices),
                    std::move(mesh.indices)
                )
            );

            mesh_handle.emplace<MeshID<VertexPNUTB>>(mesh.mesh_id);

            // Emplace bounding geometry.
            //
            // TODO: We should consider importing the scene-graph and
            // full Transform information from the assets.
            mesh_handle.emplace<LocalAABB>(mesh.aabb);
            mesh_handle.emplace<Transform>();


            if (mesh.diffuse.has_value()) {
                make_available<Binding::Texture2D>(mesh.diffuse->texture->id());
                auto& diffuse = mesh_handle.emplace<MaterialDiffuse>(mesh.diffuse->texture);

                // We check if the alpha channel even exitsts in the texture,
                // to decide on whether alpha testing should be enabled.
                PixelComponentType alpha_component =
                    diffuse.texture->get_component_type<PixelComponent::Alpha>();

                if (alpha_component != PixelComponentType::None) {
                    mesh_handle.emplace<AlphaTested>();
                }
            }

            if (mesh.specular.has_value()) {
                make_available<Binding::Texture2D>(mesh.specular->texture->id());
                mesh_handle.emplace<MaterialSpecular>(mesh.specular->texture, 128.f);
            }

            if (mesh.normal.has_value()) {
                make_available<Binding::Texture2D>(mesh.normal->texture->id());
                mesh_handle.emplace<MaterialNormal>(mesh.normal->texture);
            }


            mesh_handle.emplace<Name>(std::string(mesh.path.subpath()));
        }

    } catch (...) {
        registry.destroy(children.begin(), children.end());
        throw;
    }

    attach_children(model_handle, children);
}




} // namespace josh
