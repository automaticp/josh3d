#pragma once
#include "AABB.hpp"
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "EnumUtils.hpp"
#include "Scalars.hpp"
#include "StringHash.hpp"
#include "Transform.hpp"
#include <assimp/anim.h>
#include <assimp/camera.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <functional>


namespace josh::detail {

/*
In Assimp representation each node can be multiple entities at once:
multiple meshes, mesh+light, etc. This is rarely useful in practice.

Unitarization will duplicate the nodes to guarantee only one entity
per node in the newly constructed scene-graph.
*/
enum class Unitarization
{
    None,         // No unitarization is performed. Each node can refer to multiple entities, even of the same type.
    InsertDummy,  // Create a dummy transform node that will hold the entities *and* their children.
    UnwrapToEdge, // Create a parent-child edge from the node in arbitrary order.
};
JOSH3D_DEFINE_ENUM_EXTRAS(Unitarization, None, InsertDummy, UnwrapToEdge);

/*
Construction options for AssimpSceneRepr.
*/
struct ASRParams
{
    Unitarization unitarization = Unitarization::InsertDummy;
};

/*
The assimp scene representation is extremely inconsistent when it comes
to references between various objects. Sometimes indexing is used,
sometimes pointers, sometimes you have to look things up *by name*.
This makes working with assimp a major PITA (in addition to other issues).

Here we try to tear-down that representation and convert it to something
more consistent. We use integral IDs for referencing everything.
We also provide extra information about relationships that is not directly
accessible in aiScene (ex. Node->Bone). This, of course, comes at a cost
of extra memory. Sometimes this is referred to as the "space-sanity tradeoff".

Use IDs to index into the storage or maps. Some "private" members (prefixed with "_")
do not give a guarantee on the indexing method, but are exposed anyway.

You will likely want to enable `aiProcess_PopulateArmatureData` when importing
the scene if you will be reading any skeletal data, *especially* if you want to
import skeleton-only files.
*/
struct AssimpSceneRepr
{
    // NOTE: I've tried to make custom allocators work here, but something
    // is painfully broken in string_map with transparent lookup. No idea why.
#if 0
    using memory_resource = AsMonomorphic<std::pmr::unsynchronized_pool_resource>;
    template<typename T = void>
    using allocator = MonomorphicAllocator<memory_resource, T>;
#endif
    template<typename T = ubyte>
    using allocator = std::allocator<T>;
    using string = StringA<allocator<char>>;
    template<typename Key, typename Value>
    using map = HashMapA<Key, Value, allocator<std::pair<const Key, Value>>>;
    template<typename Key>
    using set = HashSetA<Key, allocator<Key>>;
    template<typename Value>
    using string_map = HashMapA<string, Value, allocator<std::pair<const string, Value>>, string_hash, std::equal_to<>>;
    template<typename T>
    using vector = SmallVector<T, 1, allocator<T>>;
    template<typename T>
    using span = Span<T>; // Used when assimp already represents T as an indexed array.

    // Create a repr of a loaded scene. The result is valid
    // as long as the scene is not modified or destoryed.
    [[nodiscard]]
    static auto from_scene(
        const aiScene&     ai_scene,
        const allocator<>& alloc,
        const ASRParams&   params = {})
            -> AssimpSceneRepr;

    // IDs are based on the ECS entity type.
    //
    // The registry serves as a storage for all components of the scene.
    // This includes resources that have no presence in the scene directly
    // like textures, materials, animations, etc.
    //
    // The convention is that an entitiy annotated with a given ID type
    // will always have a corresponding component in the registry.

    using registry_type = entt::registry;
    registry_type registry;

    using ID           = entt::entity;

    using MeshID       = ID; // Any mesh entity, possibly with bones.
    using LightID      = ID; // Any light entity.
    using CameraID     = ID; // Any camera eitity.

    using NodeID       = ID; // Any node of the scene-graph.
    using EntityID     = ID; // Any entity that is associated with a scene-graph node: Mesh, Light or Camera.

    using TextureID    = ID; // Any texture, possibly embedded.
    using MatID        = ID; // Any material.
    using MatTextureID = ID; // Texture as used by a material. Has extra properties like type, uvindex, etc.

    using ArmatureID   = ID; // Actual armature that we use. Will likely come from mesh bones.
    using AnimID       = ID; // Skeletal animation.

    static constexpr ID null_id = entt::null;


    // Lights and Cameras.
    // These are pretty basic wrappers around the source ptr.

    struct Light
    {
        const aiLight* ptr;
        auto operator->() const noexcept { return ptr; }
    };

    struct Camera
    {
        const aiCamera* ptr;
        auto operator->() const noexcept { return ptr; }
    };

    struct Mesh
    {
        const aiMesh* ptr;
        ArmatureID    armature_id;    // Referenced armature, if any.
        vector<u32>   boneid2jointid; // Maps a bone index to a joint index in the armature joints array.
        auto operator->() const noexcept { return ptr; }
    };

    // Materials and Textures.

    struct Texture
    {
        string           path;     // Relative path to file on disk if not embedded, or a special key if it is.
        const aiTexture* embedded; // Non-null if the texture is embedded.
    };

    struct MatTexture
    {
        TextureID     texture_id; // Referenced texture.
        aiTextureType type;       // Type of the texture, as returned by assimp.
        u32           uvindex;    // NOTE: Not used currently.
    };

    struct Material
    {
        const aiMaterial* ptr;
        string            name;
        map<aiTextureType, vector<MatTextureID>>
                          textype2mattextureids;
        auto operator->() const noexcept { return ptr; }
    };

    string_map<TextureID>  _texpath2texid; // A set of unique (by path) textures.

    // Nodes, Roots and Bones.
    //
    // NOTE: Some properties are only populated if the scene contains
    // any meshes. aiProcess_PopulateArmatureData should be enabled for
    // skeleton-only files.

    struct Node
    {
        const aiNode* ptr;          // Underlying node.
        Transform     transform;    // Parent-to-Local transform of this node.
        NodeID        parent_id;    // Parent of the node.
        NodeID        child0_id;    // First child of the node.
        NodeID        sibling_id;   // Next sibling of the node.
        u32           depth;        // 0 for root. Maybe you'll find this useful?
        EntityID      entity0_id;   // First entity in the list of referenced entities, if any.
        u32           num_entities; // Size of the entity list.
        auto operator->() const noexcept { return ptr; }
    };

    struct Entity
    {
        EntityID next_id; // Next entity in the interned list, or null_id if last.
    };

    NodeID         root_node_id;   // Root node of the scene.
    vector<NodeID> nodes_preorder; // Nodes stored in pre-order.

    map<const aiNode*, NodeID> _node2nodeid; // aiNode* -> NodeID
    string_map<const aiNode*>  _name2node;   // string -> aiNode*, to avoid O(N^2) name lookups in the bone hierarchy.
    string_map<NodeID>         name2nodeid;  // string -> NodeID, to avoid O(N^2) name lookups in the bone hierarchy.

    /*
    Bone representation is such a mess that we'll just do it ourselves.
    */
    struct Joint
    {
        string name;
        mat4   inv_bind;
        u32    parent_idx;       // Index in the `joints` array or -1 if no parent.
        u32    first_child_idx;  // Index of the first child or -1 if no children.
        u32    next_sibling_idx; // Next sibling index of the same parent or -1 if last sibling.
        u32    depth;            // Ehh, just have it here too, whatever.
        NodeID nodeid;
    };

    /*
    Armature pulled either from aiSkeleton or from the aiMesh::mBones array.
    */
    struct Armature
    {
        string           name;
        vector<Joint>    joints;  // In pre-order. First is root.
        NodeID           node_id; // Parent of the root node and the mesh node.
        map<NodeID, u32> nodeid2jointid; // Joint indices in the `joints` array.
    };

    map<NodeID, ArmatureID> rootid2armatureid; // NodeID

    // Animations.
    //
    // NOTE: Currently only skeletal animations.

    struct Animation
    {
        const aiAnimation* ptr;
        ArmatureID         armature_id;
        auto operator->() const noexcept { return ptr; }
    };


    allocator<> _allocator;

    template<typename T>
    struct Created { ID id; T& component; };

    template<typename T>
    auto _create_as(T&& component) -> Created<T>;
};


template<typename T>
auto AssimpSceneRepr::_create_as(T&& component)
    -> Created<T>
{
    const ID id = registry.create();
    T& component_ = registry.emplace<T>(id, FORWARD(component));
    return { .id=id, .component=component_ };
}


} // namespace josh::detail
