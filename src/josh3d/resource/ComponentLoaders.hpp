#pragma once
#include "AssetManager.hpp"
#include "Filesystem.hpp"
#include "GLAPIBinding.hpp"
#include "Pixels.hpp"
#include "TextureHelpers.hpp"
#include "Transform.hpp"
#include "BoundingSphere.hpp"
#include "ChildMesh.hpp"
#include "Materials.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Name.hpp"
#include "VPath.hpp"
#include "Skybox.hpp"
#include "tags/AlphaTested.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>


namespace josh {


inline Skybox& load_skybox_into(
    entt::handle handle, const File& skybox_json)
{
    auto data = load_cubemap_from_json<pixel::RGBA>(skybox_json);

    UniqueCubemap cubemap =
        create_skybox_from_cubemap_data(data, InternalFormat::SRGBA8);

    auto& skybox =
        handle.emplace_or_replace<Skybox>(std::move(cubemap));

    return skybox;
}




inline Model& emplace_model_asset_into(
    entt::handle model_handle, SharedModelAsset asset)
{
    auto& registry = *model_handle.registry();

    std::vector<entt::entity> children;

    children.resize(asset.meshes.size());
    registry.create(children.begin(), children.end());

    try {

        for (size_t i{ 0 }; i < children.size(); ++i) {

            auto& mesh        = asset.meshes[i];
            auto  mesh_handle = entt::handle(registry, children[i]);

            // Bind to make assets available in this thread.
            make_available<Binding::ArrayBuffer>       (mesh.vertices->id());
            make_available<Binding::ElementArrayBuffer>(mesh.indices->id() );

            mesh_handle.emplace<Mesh>(
                Mesh::from_buffers<VertexPNUTB>(
                    std::move(mesh.vertices),
                    std::move(mesh.indices)
                )
            );

            // Emplace bounding geometry.
            // TODO: This is terrible, use AABB
            auto [lbb, rtf] = mesh.aabb;
            float max_something = glm::max(glm::length(lbb), glm::length(rtf));
            mesh_handle.emplace<BoundingSphere>(max_something);


            if (mesh.diffuse.has_value()) {
                make_available<Binding::Texture2D>(mesh.diffuse->texture->id());
                auto& diffuse = mesh_handle.emplace<MaterialDiffuse>(mesh.diffuse->texture);

                // TODO: We check if the alpha channel even exitsts in the texture,
                // to decide on whether alpha testing should be enabled. Is there a better way?
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


            mesh_handle.emplace<ChildMesh>(model_handle.entity());

            mesh_handle.emplace<Transform>();
            mesh_handle.emplace<Name>(mesh.path.subpath);

        }

    } catch (...) {
        registry.destroy(children.begin(), children.end());
        throw;
    }

    return model_handle.emplace<Model>(std::move(children)); // noexcept, effectively
}




} // namespace josh
