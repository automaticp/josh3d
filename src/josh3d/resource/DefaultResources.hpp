#pragma once
#include "Camera.hpp"
#include "GLObjects.hpp"
#include "LODPack.hpp"
#include "LightCasters.hpp"
#include "MeshStorage.hpp"
#include "Resource.hpp"
#include "ResourceRegistry.hpp"
#include "SkeletalAnimation.hpp"
#include "Skeleton.hpp"
#include "Transform.hpp"
#include "HashedString.hpp"
#include "UUID.hpp"
#include "VertexSkinned.hpp"
#include "VertexStatic.hpp"
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>


/*
NOTE: Most of the resources must be simple reference types
without any kind of heavy data in them.
*/
namespace josh {


// "Fake enum" namespace.
namespace ResourceType_ {
constexpr ResourceTypeHS Scene     = "Scene"_hs;
constexpr ResourceTypeHS Mesh      = "Mesh"_hs;
constexpr ResourceTypeHS Texture   = "Texture"_hs;
constexpr ResourceTypeHS Animation = "Animation"_hs;
constexpr ResourceTypeHS Skeleton  = "Skeleton"_hs;
constexpr ResourceTypeHS Material  = "Material"_hs;
constexpr ResourceTypeHS MeshDesc  = "MeshDesc"_hs;
} // namespace ResourceType


namespace RT = ResourceType_;


struct SceneResource {
    struct Node {
        static constexpr int32_t no_parent = -1;
        Transform transform;
        int32_t   parent_index;

        // TODO: That UUID alignment is not nice.
        using any_type = boost::anys::basic_any<sizeof(ResourceItem) + 4>;
        HashedID object_type;
        any_type object_data;
    };

    std::shared_ptr<std::vector<Node>> nodes; // Pre-order.
};

template<> struct resource_traits<ResourceType_::Scene> {
    using resource_type = SceneResource;
};




struct TextureResource {
    SharedTexture2D texture;
};

template<> struct resource_traits<ResourceType_::Texture> {
    using resource_type = TextureResource;
};




struct SkeletonResource {
    std::shared_ptr<Skeleton> skeleton;
};

template<> struct resource_traits<ResourceType_::Skeleton> {
    using resource_type = SkeletonResource;
};




struct MeshResource {
    struct Static {
        LODPack<MeshID<VertexStatic>, 8> lods;
    };

    struct Skinned {
        LODPack<MeshID<VertexSkinned>, 8> lods;
        PrivateResource<RT::Skeleton>     skeleton;
    };

    using variant_type = std::variant<Static, Skinned>;
    variant_type mesh;
};

template<>
struct resource_traits<ResourceType_::Mesh> {
    using resource_type = MeshResource;
};




struct MeshDescResource {
    UUID  mesh_uuid;
    UUID  diffuse_uuid;
    UUID  normal_uuid;
    UUID  specular_uuid;
    float specpower;
};

template<>
struct resource_traits<ResourceType_::MeshDesc> {
    using resource_type = MeshDescResource;
};




struct AnimationResource {
    std::shared_ptr<AnimationClip> animation;
    UUID                           skeleton_uuid;
};

template<>
struct resource_traits<ResourceType_::Animation> {
    using resource_type = AnimationResource;
};



} // namespace josh
