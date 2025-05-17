#pragma once
#include "AABB.hpp"
#include "Common.hpp"
#include "GLObjects.hpp"
#include "LODPack.hpp"
#include "MeshStorage.hpp"
#include "Resource.hpp"
#include "ResourceInfo.hpp"
#include "ResourceRegistry.hpp"
#include "SkeletalAnimation.hpp"
#include "Skeleton.hpp"
#include "Transform.hpp"
#include "UUID.hpp"
#include "VertexSkinned.hpp"
#include "VertexStatic.hpp"
#include <cstdint>
#include <memory>


/*
NOTE: Most of the resources must be simple reference types
without any kind of heavy data in them.
*/
namespace josh {


/*
What a given "resource" should maybe be able to do:

    Asset -> ResourceFile                 : Be imported from an asset to file
    UUID -> ResourceFile -> Resource      : Be loaded from disk
    Resource -> (Component...)            : Be emplaced into registry as components
    (Resource, Handle) -> (Component...)  : Be used to update components
    (Component...) -> Resource            : Be recreated from components (with a provoking component)
    Resource -> ResourceFile              : Be serialized back to a file
    ResourceFile -> Asset (+Metadata)     : Be optionally re-exported back to an asset

*/


// "Fake enum" namespace.
namespace ResourceType_ {
constexpr ResourceTypeHS Scene       = "Scene"_hs;
constexpr ResourceTypeHS StaticMesh  = "StaticMesh"_hs;
constexpr ResourceTypeHS SkinnedMesh = "SkinnedMesh"_hs;
constexpr ResourceTypeHS Texture     = "Texture"_hs;
constexpr ResourceTypeHS Animation   = "Animation"_hs;
constexpr ResourceTypeHS Skeleton    = "Skeleton"_hs;
constexpr ResourceTypeHS Material    = "Material"_hs;
constexpr ResourceTypeHS MeshDesc    = "MeshDesc"_hs; // TODO: This name is shaky.
} // namespace ResourceType


namespace RT = ResourceType_;


struct SceneResource {
    struct Node {
        static constexpr int32_t no_parent = -1;
        Transform transform;
        int32_t   parent_index;
        UUID      uuid;
    };

    using node_list_type = Vector<Node>;
    std::shared_ptr<node_list_type> nodes; // Pre-order.
};
template<> struct resource_traits<RT::Scene> { using resource_type = SceneResource; };


struct SkeletonResource {
    std::shared_ptr<Skeleton> skeleton;
};
template<> struct resource_traits<RT::Skeleton> { using resource_type = SkeletonResource; };


struct AnimationResource {
    using keyframes_type = AnimationClip::JointKeyframes;
    std::shared_ptr<Vector<keyframes_type>> keyframes;
    double                                  duration_s;
    UUID                                    skeleton_uuid;
};
template<> struct resource_traits<RT::Animation> { using resource_type = AnimationResource; };


struct StaticMeshResource {
    LODPack<MeshID<VertexStatic>, 8> lods;
    LocalAABB                        aabb;
};
template<> struct resource_traits<RT::StaticMesh> { using resource_type = StaticMeshResource; };


struct SkinnedMeshResource {
    LODPack<MeshID<VertexSkinned>, 8> lods;
    LocalAABB                         aabb;
    UUID                              skeleton_uuid;
};
template<> struct resource_traits<RT::SkinnedMesh> { using resource_type = SkinnedMeshResource; };


struct TextureResource {
    SharedTexture2D texture;
};
template<> struct resource_traits<RT::Texture> { using resource_type = TextureResource; };


struct MaterialResource {
    UUID  diffuse_uuid; // Textures
    UUID  normal_uuid;
    UUID  specular_uuid;
    float specpower;
};
template<> struct resource_traits<RT::Material> { using resource_type = MaterialResource; };


/*
TODO: Ultimately, this is a crappy stand-in for a more general "entity"
that can contain an arbitrary number of components by means of referencing
multiple UUIDs, possibly based on a prefab of some kind.
*/
struct MeshDescResource {
    UUID mesh_uuid;
    UUID material_uuid;
};
template<> struct resource_traits<RT::MeshDesc> { using resource_type = MeshDescResource; };




inline void register_default_resource_info(
    ResourceInfo& m)
{
    m.register_resource_type<RT::Scene>();
    m.register_resource_type<RT::Material>();
    m.register_resource_type<RT::MeshDesc>();
    m.register_resource_type<RT::StaticMesh>();
    m.register_resource_type<RT::SkinnedMesh>();
    m.register_resource_type<RT::Texture>();
    m.register_resource_type<RT::Skeleton>();
    m.register_resource_type<RT::Animation>();
}


inline void register_default_resource_storage(
    ResourceRegistry& r)
{
    r.initialize_storage_for<RT::Scene>();
    r.initialize_storage_for<RT::MeshDesc>();
    r.initialize_storage_for<RT::Material>();
    r.initialize_storage_for<RT::StaticMesh>();
    r.initialize_storage_for<RT::SkinnedMesh>();
    r.initialize_storage_for<RT::Texture>();
    r.initialize_storage_for<RT::Skeleton>();
    r.initialize_storage_for<RT::Animation>();
}


} // namespace josh
