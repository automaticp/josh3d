#include "AssimpSceneRepr.hpp"
#include "Common.hpp"
#include "ContainerUtils.hpp"
#include "Ranges.hpp"
#include "detail/AssimpCommon.hpp"
#include <assimp/anim.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <iterator>
#include <tuple>
#include <utility>


/*
NOTE: I'm playing around with a slightly different formatting style
in this file. Formatting of braces/blocks, is not going to be consistent
with the other code.
*/
namespace josh::detail {
namespace {

using asr = AssimpSceneRepr;
using NodeID = asr::NodeID;

auto populate_nodes(
    AssimpSceneRepr&                 self,
    const aiNode*                    ai_node,
    u32                              depth,
    asr::map<const aiNode*, NodeID>& node2nodeid,
    asr::string_map<const aiNode*>&  name2node,
    const asr::allocator<>&          alloc)
        -> NodeID
{
    assert(ai_node);

    const NodeID node_id = self._create_as<asr::Node>({
        .ptr          = ai_node,
        .transform    = m2tf(ai_node->mTransformation),
        .parent_id    = asr::null_id, // Will set later during unwinding.
        .child0_id    = asr::null_id, // ''
        .sibling_id   = asr::null_id, // ''
        .depth        = depth,
        .entity0_id   = asr::null_id, // Will set much later.
        .num_entities = 0,            // ''
    }).id;

    node2nodeid.emplace(ai_node, node_id);

    if (const auto& name = ai_node->mName; name.length)
    {
        // NOTE: Only guaranteed to be unique for bone/animated nodes.
        name2node.emplace(std::piecewise_construct, std::forward_as_tuple(name.C_Str(), alloc), std::tuple(ai_node));
    }

    NodeID prev_sibling_id = asr::null_id;
    for (const aiNode* child_node : make_span(ai_node->mChildren, ai_node->mNumChildren))
    {
        const NodeID child_id =
            populate_nodes(self, child_node, depth + 1, node2nodeid, name2node, alloc);

        auto& node  = self.registry.get<asr::Node>(node_id);
        auto& child = self.registry.get<asr::Node>(child_id);

        child.parent_id = node_id;

        if (node.child0_id == asr::null_id)
            node.child0_id = child_id;

        if (prev_sibling_id != asr::null_id)
            self.registry.get<asr::Node>(prev_sibling_id).sibling_id = child_id;

        prev_sibling_id = child_id;
    }

    return node_id;
}

void push_front(
    asr::registry_type& registry,
    asr::Node&          node,
    asr::EntityID       new_id)
{
    registry.emplace<asr::Entity>(new_id, node.entity0_id);
    node.entity0_id = new_id;
    node.num_entities += 1;
}

[[nodiscard]]
auto pop_front(
    asr::registry_type& registry,
    asr::Node&          node)
        -> asr::EntityID
{
    assert(node.num_entities and node.child0_id != asr::null_id);
    const asr::EntityID old_child0 = node.child0_id;
    const asr::EntityID new_child0 = registry.get<asr::Entity>(old_child0).next_id;
    node.child0_id = new_child0;
    node.num_entities -= 1;
    registry.erase<asr::Entity>(old_child0);
    return old_child0;
}

void unitarize_nodes(
    AssimpSceneRepr& self,
    NodeID           node_id,
    u32              depth,
    Unitarization    unitarization)
{
    if (unitarization == Unitarization::None) return;

    auto& node = self.registry.get<asr::Node>(node_id);
    node.depth = depth; // It's easier to overwrite this everywhere than to bother.

    if (node.num_entities > 1)
    {
        // Time to get our hands dirty.
        if (unitarization == Unitarization::InsertDummy)
        {
            // Given that the number of entities in the node is N,
            // create N child leaf nodes and move each entity into
            // them one-to-one. The transform is preserved for this
            // node, and the transforms of the new children are I.
            while (node.num_entities > 0)
            {
                const auto [new_child_id, new_child] = self._create_as<asr::Node>({
                    // FIXME: It's probably best to scrap all of the aiNode data and
                    // not refer to it at all.
                    .ptr          = nullptr,
                    .transform    = {},
                    .parent_id    = node_id,
                    .child0_id    = asr::null_id,
                    .sibling_id   = node.child0_id,
                    .depth        = node.depth + 1,
                    .entity0_id   = asr::null_id,   // Will push_front().
                    .num_entities = 0,              // ''
                });
                node.child0_id = new_child_id;

                push_front(self.registry, new_child, pop_front(self.registry, node));
            }
        }

        if (unitarization == Unitarization::UnwrapToEdge)
        {
            // Given N entitites in the node, create child a node,
            // then a child of child, then a child of that, etc.
            // until there's a node per entity (N-1 descendents total).
            //
            // NOTE: The resulting order does not matter since the order
            // in the original entities list is just as arbitrary.
            //
            // NOTE: This is where the `depth` value will diverge in weird
            // ways, hence why we forcefully overwrite it above.
            NodeID parent_id = node_id;
            while (node.num_entities > 1)
            {
                auto& parent = self.registry.get<asr::Node>(parent_id);
                const auto [new_child_id, new_child] = self._create_as<asr::Node>({
                    .ptr          = nullptr,
                    .transform    = {},
                    .parent_id    = parent_id,
                    .child0_id    = parent.child0_id, // Karen takes the children.
                    .sibling_id   = asr::null_id,
                    .depth        = {},               // Will be overwrittten.
                    .entity0_id   = asr::null_id,
                    .num_entities = 0,
                });
                parent.child0_id = new_child_id;
                parent_id        = new_child_id;

                // NOTE: Pop from the node, it has the full list, not the parent.
                push_front(self.registry, new_child, pop_front(self.registry, node));
            }
        }
    }

    NodeID child_id = node.child0_id;
    while (child_id != asr::null_id)
    {
        unitarize_nodes(self, child_id, depth + 1, unitarization);
        child_id = self.registry.get<asr::Entity>(child_id).next_id;
    }
}

void populate_nodes_preorder(
    const AssimpSceneRepr& self,
    NodeID                 node_id,
    asr::vector<NodeID>&   nodes_preorder)
{
    // This is fairly simple once the whole graph is built.
    nodes_preorder.push_back(node_id);

    const auto& node = self.registry.get<asr::Node>(node_id);

    NodeID child_id = node.child0_id;
    while (child_id != asr::null_id)
    {
        populate_nodes_preorder(self, child_id, nodes_preorder);
        child_id = self.registry.get<asr::Node>(child_id).sibling_id;
    }
}

// NOTE: The armature node itself must not be included.
template<typename aiBoneT>
void populate_armature_preorder(
    const aiNode*                                  ai_node,
    u32                                            depth,
    asr::vector<asr::Joint>&                       joints,
    asr::map<asr::NodeID, u32>&                    nodeid2joint_idx,
    const asr::map<const aiNode*, asr::NodeID>&    node2nodeid,
    const asr::map<const aiNode*, const aiBoneT*>& node2bone,
    const asr::allocator<>&                        alloc)
{
    assert(ai_node);

    // Have to check that the child still belongs to the skeleton structure.
    // If not, we skip over that subtree, that kind of setup is too weird.
    const aiBoneT* const* ai_bone = try_find_value(node2bone, ai_node);
    if (not ai_bone) return;

    const NodeID* node_id = try_find_value(node2nodeid, ai_node);
    assert(node_id);

    const uindex joint_idx = joints.size();
    joints.push_back({
        .name             = asr::string(ai_node->mName.C_Str(), alloc),
        .inv_bind         = m2m((*ai_bone)->mOffsetMatrix),
        .parent_idx       = u32(-1), // Will set later when unwinding.
        .first_child_idx  = u32(-1), // ...
        .next_sibling_idx = u32(-1), // ...
        .depth            = depth,
        .nodeid           = *node_id,
    });

    auto [it, was_emplaced] = nodeid2joint_idx.emplace(*node_id, joint_idx);
    assert(was_emplaced);

    u32 prev_sibling_idx = -1;
    for (const aiNode* child_node : make_span(ai_node->mChildren, ai_node->mNumChildren))
    {
        const u32 child_idx = joints.size();
        populate_armature_preorder(child_node, depth + 1, joints, nodeid2joint_idx, node2nodeid, node2bone, alloc);

        // Fix-up the relationships.
        auto& child = joints[child_idx]; // Index again, as it could be invalidated in populate().
        auto& joint = joints[joint_idx]; // ...

        child.parent_idx = joint_idx;

        if (joint.first_child_idx == u32(-1))
            joint.first_child_idx = child_idx;

        if (prev_sibling_idx != u32(-1))
            joints[prev_sibling_idx].next_sibling_idx = child_idx;

        prev_sibling_idx = child_idx;
    }
}


} // namespace


auto AssimpSceneRepr::from_scene(
    const aiScene&     ai_scene,
    const allocator<>& alloc,
    const ASRParams&   params)
        -> AssimpSceneRepr
{
    auto self = AssimpSceneRepr{
        .registry = {},
        ._texpath2texid{ alloc },
        ._node2nodeid     { alloc },
        ._name2node       { alloc },
        .rootid2armatureid{ alloc },
        ._allocator = alloc,
    };

    const auto ai_meshes    = make_span(ai_scene.mMeshes,     ai_scene.mNumMeshes);
    const auto ai_materials = make_span(ai_scene.mMaterials,  ai_scene.mNumMaterials);
    const auto ai_lights    = make_span(ai_scene.mLights,     ai_scene.mNumLights);
    const auto ai_cameras   = make_span(ai_scene.mCameras,    ai_scene.mNumCameras);
    const auto ai_skeletons = make_span(ai_scene.mSkeletons,  ai_scene.mNumSkeletons);
    const auto ai_anims     = make_span(ai_scene.mAnimations, ai_scene.mNumAnimations);

    // Populate Nodes first, so that we could relate all entities to
    // their respective nodes. This will be followed by unitarization,
    // which will possibly alter the scene graph. Only after that
    // we can proceed to populating other graph-dependent components.

    populate_nodes(self, ai_scene.mRootNode, 0, self._node2nodeid, self._name2node, alloc);
    self.root_node_id = self._node2nodeid.at(ai_scene.mRootNode);

    // NOTE: name2nodeid will not reach new nodes created during unitarization.
    // But it will likely not be needed at that point, since there will be more
    // sane ways to look things up.
    for (const auto& [name, node] : self._name2node)
        self.name2nodeid.emplace(name, self._node2nodeid.at(node));

    // Entities: Meshes, Lights and Cameras.

    // NOTE: Nodes store lists of Meshes directly, but Lights and Cameras
    // have to be looked up by name. Consistency is not assimp's strongest suit.

    map<const aiMesh*, MeshID> _mesh2meshid; // Will need this for node entity lists.

    for (const aiMesh* ai_mesh : ai_meshes)
    {
        const MeshID mesh_id = self._create_as<Mesh>({
            .ptr            = ai_mesh,
            .armature_id    = asr::null_id, // Will be filled during armature population.
            .boneid2jointid = {},           // ''
        }).id;
        _mesh2meshid.emplace(ai_mesh, mesh_id);
    }

    for (const aiLight* ai_light : ai_lights)
    {
        const LightID light_id = self._create_as<Light>({ .ptr=ai_light }).id;
        const NodeID  node_id  = self.name2nodeid.at(s2sv(ai_light->mName));
        auto&         node     = self.registry.get<Node>(node_id);
        push_front(self.registry, node, light_id);
    }

    for (const aiCamera* ai_camera : ai_cameras)
    {
        const CameraID camera_id = self._create_as<Camera>({ .ptr=ai_camera }).id;
        const NodeID   node_id   = self.name2nodeid.at(s2sv(ai_camera->mName));
        auto&          node      = self.registry.get<Node>(node_id);
        push_front(self.registry, node, camera_id);
    }

    // Now resolve meshes by iterating over all nodes again.
    // This could be done in a single pass in populate() but it
    // does too much stuff at once already.

    for (const auto [node_id, node] : self.registry.view<Node>().each())
    {
        // Oh, honey, look, now it's a lookup by index!
        const auto mesh_idxs = make_span(node->mMeshes, node->mNumMeshes);
        for (const u32 mesh_idx : mesh_idxs)
        {
            const aiMesh* ai_mesh = ai_meshes[mesh_idx];
            const MeshID  mesh_id = _mesh2meshid.at(ai_mesh);
            push_front(self.registry, node, mesh_id);
        }
    }

    // Now that we know full enitity lists we can do a unitarization pass.

    unitarize_nodes(self, self.root_node_id, 0, params.unitarization);

    // Finally, we can do a final preorder tranversal so that you don't
    // have to. This could be merged with unitarization and populate()
    // but again, I'd rather not make a mess for peanuts.

    populate_nodes_preorder(self, self.root_node_id, self.nodes_preorder);

    // Materials and Textures.

    for (const aiMaterial* ai_material : ai_materials)
    {
        const aiString name = ai_material->GetName();
        auto [materialid, material] = self._create_as<Material>({
            .ptr  = ai_material,
            .name = string(name.data, name.length, alloc),
            .textype2mattextureids{ alloc }, // Fill below.
        });
        auto& textype2mattextureids = material.textype2mattextureids;

        for (const auto type_idx : irange<i32>(AI_TEXTURE_TYPE_MAX))
        {
            const auto type = aiTextureType(type_idx);
            const auto num_textures = ai_material->GetTextureCount(type);

            if (num_textures)
            {
                auto [it, was_emplaced] = textype2mattextureids.try_emplace(type, alloc);
                assert(was_emplaced);
                auto& mattextureids = it->second;

                for (const auto tex_idx : irange(num_textures))
                {
                    aiString path;
                    u32      uvindex;
                    ai_material->GetTexture(type, tex_idx, &path, nullptr, &uvindex);
                    const aiTexture* embedded = ai_scene.GetEmbeddedTexture(path.C_Str());

                    // NOTE: Will create a new entity only if the texture path is not in the map.
                    // The corresponding TextureID will be returned through the iteator either way.

                    auto it = self._texpath2texid.find(s2sv(path));
                    if (it == self._texpath2texid.end()) // Will emplace.
                    {
                        const TextureID new_textureid = self._create_as<Texture>({
                            .path     = string(path.data, path.length, alloc),
                            .embedded = embedded,
                        }).id;

                        std::tie(it, std::ignore) =
                            self._texpath2texid.emplace(string(s2sv(path), alloc), new_textureid);
                    }
                    const TextureID textureid = it->second;

                    const MatTextureID new_mattextureid = self._create_as<MatTexture>({
                        .texture_id = textureid,
                        .type      = type,
                        .uvindex   = uvindex,
                    }).id;

                    mattextureids.push_back(new_mattextureid);
                }
            }
        }
    } // for (ai_material)


    // Roots, Armatures and Bones.

    // The problem is that the mBones array is not guaranteed to store the
    // root as the first member, and if the PopulateArmatureData is not set,
    // then the mArmature field is not set either.
    //
    // To find the root bone/node per mesh we collect a set of bones per mesh,
    // then take an arbitrary bone from the mesh, find its node, and walk upwards
    // until no node-associated bone is in the same set. Then we have reached the root.
    //
    // NOTE: We have to make "an assumption" to keep our sanity and properly deal
    // with skeleton instancing:
    //   - We assume that each root node uniquely identifies a single armature.
    //   - We assume that an mBones array of *any mesh* can be used to infer the
    //     inverse bind matrices for the respective armature.
    //   - Multiple skeletons never share/alias nodes between them.

    // "Root Node" node refers to the root bone node of the hierarchy.
    // "Armature Node" is a node one above the root, used to identify a skeleton.
    // We use this node to identify a skeleton.
    // "Aramture" itself is not a node, but a skeleton resource, transformed
    // into a sane format. We use ArmatureID everywhere to refer to skeletons,
    // while the SkeletonID is assimp specific and not always present.

    map<uindex, map<const aiNode*, const aiSkeletonBone*>> _skeletonidx2_node2skbone{ alloc };

    // We go through the list of skeletons and get the armatures for them.
    // This step is optional, as the `skeletons` only has contents if the flag
    // PopulateArmatureData is set. If no flag, then we will get the armatures
    // in the following loop over meshes instead, this will skip mesh-less
    // skeletons in a given file.
    for (const auto [skeleton_idx, ai_skeleton] : enumerate(ai_skeletons))
    {
        const auto ai_skbones = make_span(ai_skeleton->mBones, ai_skeleton->mNumBones);
        const aiSkeletonBone* ai_skbone0 = ai_skbones[0];

        // If the PopulateArmatureData flag is not set, these will be null.
        // In that case, we have no way of referring these skeletons back
        // to the meshes that use them with proper skeleton instancing,
        // so we have to skip.
        //
        // NOTE: I am not sure if it even possible to have the skeletons array
        // without the PopulateArmatureData flag. The docs on this are nonexistent.
        // Why is assimp like this? Abandoned, desolate and hopeless...
        if (not ai_skbone0->mArmature or not ai_skbone0->mNode) continue;

        // We assume here that the armature node has only a single child -
        // the root of the skeleton.
        //
        // TODO: Uhh, why would that be guaranteed?
        const aiNode* armature_node = ai_skbone0->mArmature;
        assert(armature_node->mChildren and armature_node->mNumChildren == 1);

        const aiNode* root_node  = armature_node->mChildren[0];
        const NodeID  root_id    = self._node2nodeid[root_node];
        const NodeID  armnode_id = self._node2nodeid[armature_node]; // NOT the same as ArmatureID.

        // This one is a pain, since each skeleton does not store the bones
        // in any predetermined order. Doing a pre-order iteration over bones
        // becomes unnecessarily complicated as we have to precompute bone -> node. Ohwell.

        auto [it1, was_emplaced1] = _skeletonidx2_node2skbone.try_emplace(skeleton_idx, alloc);
        map<const aiNode*, const aiSkeletonBone*>& _node2skbone = it1->second;
        assert(was_emplaced1);
        for (const aiSkeletonBone* ai_skbone : ai_skbones)
        {
            assert(ai_skbone->mNode);
            _node2skbone.try_emplace(ai_skbone->mNode, ai_skbone);
        }

        auto it = self.rootid2armatureid.find(root_id);
        if (it == self.rootid2armatureid.end())
        {
            auto joints         = vector<Joint>(alloc);
            auto nodeid2jointid = map<NodeID, u32>(alloc);

            populate_armature_preorder(root_node, 0, joints, nodeid2jointid,
                self._node2nodeid, _node2skbone, alloc);

            auto name = eval%[&]() -> string {
                if (ai_skeleton->mName.length) return string(ai_skeleton->mName.C_Str(), alloc);
                else                           return string(armature_node->mName.C_Str(), alloc);
            };

            const ArmatureID armature_id = self._create_as<Armature>({
                .name           = MOVE(name),
                .joints         = MOVE(joints),
                .node_id        = armnode_id,
                .nodeid2jointid = MOVE(nodeid2jointid),
            }).id;

            self.rootid2armatureid.emplace(root_id, armature_id);
        }
    }

    map<MeshID, map<const aiBone*, const aiNode*>> _meshid2_bone2node { alloc }; // Implicitly a set of bones per mesh.
    map<MeshID, map<const aiNode*, const aiBone*>> _meshid2_node2bone { alloc };
    map<MeshID, const aiNode*>                     _meshid2rootnode   { alloc };

    // Get the armatures from meshes as a fallback.
    for (const auto [mesh_id, mesh] : self.registry.view<Mesh>().each())
    {
        if (mesh->HasBones())
        {
            // Bones from different meshes can refer to the same nodes, and by extension
            // to the same skeleton. We take a set of bones as a per-mesh property.
            const auto ai_bones = make_span(mesh->mBones, mesh->mNumBones);

            auto [it1, was_emplaced1] = _meshid2_bone2node.try_emplace(mesh_id, alloc);
            auto [it2, was_emplaced2] = _meshid2_node2bone.try_emplace(mesh_id, alloc);
            assert(was_emplaced1);
            assert(was_emplaced2);
            map<const aiBone*, const aiNode*>& _bone2node = it1->second;
            map<const aiNode*, const aiBone*>& _node2bone = it2->second;

            for (const aiBone* ai_bone : ai_bones)
            {
                // NOTE: The lookup by name is awful. Use PopulateArmatureData to avoid it.
                const aiNode* ai_node = eval%[&]() -> const aiNode* {
                    if (ai_bone->mNode) return ai_bone->mNode;
                    else                return self._name2node.at(s2sv(ai_bone->mName));
                };

                _bone2node.emplace(ai_bone, ai_node);
                _node2bone.emplace(ai_node, ai_bone);
            }

            // Ok, now we can do the root finding.

            // This will set ai_node to the root of the bone hierarchy, and
            // ai_node_parent to the node above it, indicating the armature.
            // This is similar to what the mArmature field would contain otherwise.
            //
            // TODO: Right now we do this ourselves and assert it by comparing the
            // result with the mArmature field. If this checks out, we should
            // remove the redundant computation and use mArmature if present.

            const aiBone* ai_bone0       = ai_bones[0];
            const aiNode* ai_node_parent = _bone2node.at(ai_bone0);
            const aiBone* ai_bone_parent = ai_bone0;
            const aiNode* ai_node;
            const aiBone* ai_bone;
            do
            {
                ai_node = ai_node_parent;
                ai_bone = ai_bone_parent;

                ai_node_parent = ai_node->mParent;
                const aiBone** maybe_ai_bone = try_find_value(_node2bone, ai_node_parent);
                ai_bone_parent = maybe_ai_bone ? *maybe_ai_bone : nullptr;
            }
            while (_bone2node.contains(ai_bone_parent));

            assert(ai_node_parent && "Should have at least one node above.");
            assert(ai_bone_parent == nullptr);
            if (ai_bone0->mArmature)
                assert(ai_bone0->mArmature == ai_node_parent && "Wrong assumption about mArmature");

            // At this point ai_node and ai_bone refer to the root node of the skeleton.
            // We can use this information to descend the hierarchy again and populate
            // armature joint data. We only do this once per root.
            const aiNode* root_node      = ai_node;
            const aiNode* armature_node  = ai_node_parent;
            const NodeID  root_id        = self._node2nodeid[root_node];
            const NodeID  armnode_id     = self._node2nodeid[armature_node]; // NOT the same as ArmatureID.

            auto it = self.rootid2armatureid.find(root_id);
            if (it == self.rootid2armatureid.end())
            {
                // The current mesh's node2bone will be used for inv-bind data.
                // Assuming the bind pose is the same for all meshes that use the same skeleton.

                auto joints         = vector<Joint>(alloc);
                auto nodeid2jointid = map<NodeID, u32>(alloc);

                populate_armature_preorder(root_node, 0, joints, nodeid2jointid,
                    self._node2nodeid, _node2bone, alloc);

                const ArmatureID armature_id = self._create_as<Armature>({
                    .name           = string(armature_node->mName.C_Str(), alloc),
                    .joints         = MOVE(joints),
                    .node_id        = armnode_id,
                    .nodeid2jointid = MOVE(nodeid2jointid),
                }).id;

                std::tie(it, std::ignore) =
                    self.rootid2armatureid.emplace(root_id, armature_id);
            }
            const ArmatureID armature_id = it->second;

            // Record remaining info.
            _meshid2rootnode.try_emplace(mesh_id, root_node);
            mesh.armature_id = armature_id;

            // Remember to fill out the bone to joint mapping.
            assert(mesh.boneid2jointid.empty());
            for (const aiBone* ai_bone : ai_bones)
            {
                const aiNode* ai_node   = _bone2node.at(ai_bone);
                const NodeID  node_id   = self._node2nodeid.at(ai_node);
                const u32     joint_idx = self.registry.get<Armature>(armature_id).nodeid2jointid.at(node_id);
                mesh.boneid2jointid.emplace_back(joint_idx);
            }
        }
    }

    // Find which animations reference which armature.
    //
    // NOTE: We have to make more assumptions here:
    //   - A single animation only affects nodes that correspond to a single skeleton.
    //     I.e., there are no multi-skeleton, node-only or nodes+skeleton animations
    //     (we might try to handle node-only and node+skeleton animation later; it's a PITA).

    for (const aiAnimation* ai_anim : ai_anims)
    {
        const auto [anim_id, anim] = self._create_as<Animation>({
            .ptr         = ai_anim,
            .armature_id = asr::null_id, // Will set later.
        });

        const auto joint_motions = make_span(ai_anim->mChannels, ai_anim->mNumChannels);
        assert(joint_motions.size());
        const auto janim0 = joint_motions[0]; // Grab any joint for probing.
        const aiNode* affected_node   = self._name2node.at(s2sv(janim0->mNodeName));
        const NodeID  affected_nodeid = self._node2nodeid.at(affected_node);

        for (const auto [armature_id, armature] : self.registry.view<Armature>().each())
        {
            if (armature.nodeid2jointid.contains(affected_nodeid))
            {
                assert(anim.armature_id == asr::null_id);
                anim.armature_id = armature_id;
            }
        }
        assert(anim.armature_id != asr::null_id);
    }

    return self;
}


} // namespace josh::detail
