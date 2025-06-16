#include "ComponentLoaders.hpp"
#include "Asset.hpp"
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

    try {

        for (size_t i{ 0 }; i < children.size(); ++i) {

            const Handle mesh_handle{ registry, children[i] };

            const auto emplace_mesh_from_asset = [&]<typename T>(T& mesh_asset) {
                using vertex_type = T::vertex_type;

                // Bind to make assets available in this thread.
                make_available<Binding::ArrayBuffer>       (mesh_asset.vertices->id());
                make_available<Binding::ElementArrayBuffer>(mesh_asset.indices->id() );


                if constexpr (std::same_as<T, SharedMeshAsset>) {

                    mesh_handle.emplace<Mesh>(
                        Mesh::from_buffers<vertex_type>(MOVE(mesh_asset.vertices), MOVE(mesh_asset.indices)));
                    mesh_handle.emplace<MeshID<vertex_type>>(mesh_asset.mesh_id);

                } else if constexpr (std::same_as<T, SharedSkinnedMeshAsset>) {

                    const auto& skeleton_asset = mesh_asset.skeleton_asset;

                    mesh_handle.emplace<SkinnedMesh>(mesh_asset.mesh_id, skeleton_asset.skeleton);

                    // HACK: Directly emplacing animations into a mesh entity.
                    using ranges::to, std::views::transform;
                    auto anims =
                        mesh_asset.animation_assets |
                        transform(&SharedAnimationAsset::animation) |
                        to<std::vector<std::shared_ptr<const AnimationClip>>>();

                    mesh_handle.emplace<MeshAnimations>(MOVE(anims));

                } else { JOSH3D_STATIC_ASSERT_FALSE(T); }

                // Emplace bounding geometry.
                //
                // TODO: We should consider importing the scene-graph and
                // full Transform information from the assets.
                mesh_handle.emplace<LocalAABB>(mesh_asset.aabb);
                mesh_handle.emplace<Transform>();

                if (auto* mat_diffuse = try_get(mesh_asset.diffuse)) {
                    make_available<Binding::Texture2D>(mat_diffuse->texture->id());
                    mesh_handle.emplace<MaterialDiffuse>(mat_diffuse->texture, ResourceUsage());

                    // We check if the alpha channel even exitsts in the texture,
                    // to decide on whether alpha testing should be enabled.
                    PixelComponentType alpha_component =
                        mat_diffuse->texture->template get_component_type<PixelComponent::Alpha>();

                    if (alpha_component != PixelComponentType::None) {
                        mesh_handle.emplace<AlphaTested>();
                    }
                }

                if (auto* mat_specular = try_get(mesh_asset.specular)) {
                    make_available<Binding::Texture2D>(mat_specular->texture->id());
                    mesh_handle.emplace<MaterialSpecular>(mat_specular->texture, ResourceUsage(), 128.f);
                }

                if (auto* mat_normal = try_get(mesh_asset.normal)) {
                    make_available<Binding::Texture2D>(mat_normal->texture->id());
                    mesh_handle.emplace<MaterialNormal>(mat_normal->texture, ResourceUsage());
                }

                mesh_handle.emplace<Name>(std::string(mesh_asset.path.subpath()));
            };


            visit(emplace_mesh_from_asset, model_asset.meshes[i]);
        }

    } catch (...) {
        registry.destroy(children.begin(), children.end());
        throw;
    }

    attach_children(model_handle, children);
}


} // namespace josh
