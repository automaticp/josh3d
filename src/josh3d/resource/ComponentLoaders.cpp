#include "ComponentLoaders.hpp"
#include "Asset.hpp"
#include "Components.hpp"
#include "ContainerUtils.hpp"
#include "CubemapData.hpp"
#include "ECS.hpp"
#include "Filesystem.hpp"
#include "GLAPIBinding.hpp"
#include "GLObjects.hpp"
#include "Pixels.hpp"
#include "SceneGraph.hpp"
#include "SkeletalAnimation.hpp"
#include "SkinnedMesh.hpp"
#include "StaticMesh.hpp"
#include "TextureHelpers.hpp"
#include "Transform.hpp"
#include "Materials.hpp"
#include "Mesh.hpp"
#include "Name.hpp"
#include "Skybox.hpp"
#include "detail/StaticAssertFalseMacro.hpp"
#include "AlphaTested.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <range/v3/range/conversion.hpp>


namespace josh {


auto load_skybox_into(
    Handle      handle,
    const File& skybox_json)
        -> Skybox&
{
    CubemapPixelData<pixel::RGBA> data =
        load_cubemap_pixel_data_from_json<pixel::RGBA>(skybox_json);

    UniqueCubemap cubemap =
        create_skybox_from_cubemap_pixel_data(data, InternalFormat::SRGBA8);

    Skybox& skybox =
        handle.emplace_or_replace<Skybox>(std::move(cubemap));

    return skybox;
}


void emplace_model_asset_into(
    Handle           model_handle,
    SharedModelAsset model_asset)
{
    auto& registry = *model_handle.registry();

    thread_local std::vector<entt::entity> children;
    children.resize(model_asset.meshes.size());
    registry.create(children.begin(), children.end());

    try
    {
        for (size_t i{ 0 }; i < children.size(); ++i)
        {
            const Handle mesh_handle = { registry, children[i] };

            const auto emplace_mesh_from_asset = [&]<typename T>(T& mesh_asset)
            {
                using vertex_type = T::vertex_type;

                // Bind to make assets available in this thread.
                glapi::make_available<Binding::ArrayBuffer>       (mesh_asset.vertices->id());
                glapi::make_available<Binding::ElementArrayBuffer>(mesh_asset.indices->id() );

                if constexpr (std::same_as<T, SharedMeshAsset>)
                {
                    insert_component<StaticMesh>(mesh_handle, {
                        .lods = { .lods={ mesh_asset.mesh_id } },
                    });
                    // TODO: Remove these representations.
                    mesh_handle.emplace<Mesh>(
                        Mesh::from_buffers<vertex_type>(MOVE(mesh_asset.vertices), MOVE(mesh_asset.indices)));
                    mesh_handle.emplace<MeshID<vertex_type>>(mesh_asset.mesh_id);
                }
                else if constexpr (std::same_as<T, SharedSkinnedMeshAsset>)
                {
                    // NOTE: Below is old an not supported anymore.
                    const auto& skeleton_asset = mesh_asset.skeleton_asset;

                    mesh_handle.emplace<SkinnedMesh>(mesh_asset.mesh_id, skeleton_asset.skeleton);

                    // HACK: Directly emplacing animations into a mesh entity.
                    using ranges::to, std::views::transform;
                    auto anims =
                        mesh_asset.animation_assets |
                        transform(&SharedAnimationAsset::animation) |
                        to<std::vector<std::shared_ptr<const AnimationClip>>>();

                    mesh_handle.emplace<MeshAnimations>(MOVE(anims));
                }
                else JOSH3D_STATIC_ASSERT_FALSE(T);

                // Emplace bounding geometry.
                //
                // TODO: We should consider importing the scene-graph and
                // full Transform information from the assets.
                mesh_handle.emplace<LocalAABB>(mesh_asset.aabb);
                mesh_handle.emplace<Transform>();

                auto& mtl = insert_component(mesh_handle, make_default_material_phong());

                if (auto* diffuse = try_get(mesh_asset.diffuse))
                {
                    glapi::make_available<Binding::Texture2D>(diffuse->texture->id());
                    mtl.diffuse = diffuse->texture;

                    // We check if the alpha channel even exitsts in the texture,
                    // to decide on whether alpha testing should be enabled.
                    PixelComponentType alpha_component =
                        mtl.diffuse->template get_component_type<PixelComponent::Alpha>();

                    if (alpha_component != PixelComponentType::None)
                        mesh_handle.emplace<AlphaTested>();
                }

                if (auto* specular = try_get(mesh_asset.specular))
                {
                    glapi::make_available<Binding::Texture2D>(specular->texture->id());
                    mtl.specular = specular->texture;
                    // NOTE: Specpower is kept default ;_;.
                }

                if (auto* normal = try_get(mesh_asset.normal))
                {
                    glapi::make_available<Binding::Texture2D>(normal->texture->id());
                    mtl.normal = normal->texture;
                }

                mesh_handle.emplace<Name>(std::string(mesh_asset.path.subpath()));
            };

            visit(emplace_mesh_from_asset, model_asset.meshes[i]);
        }
    }
    catch (...)
    {
        registry.destroy(children.begin(), children.end());
        throw;
    }

    attach_children(model_handle, children);
}


} // namespace josh
