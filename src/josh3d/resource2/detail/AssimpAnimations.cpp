#include "AssimpCommon.hpp"
#include "AssetImporter.hpp"
#include "Ranges.hpp"
#include <assimp/scene.h>
#include <assimp/mesh.h>


namespace josh::detail {
namespace {


void populate_joints_preorder(
    std::vector<Joint>&                                     joints,
    std::vector<ResourceName>&                              joint_names,
    std::unordered_map<const aiNode*, size_t>&              node2id,
    const std::unordered_map<const aiNode*, const aiBone*>& node2bone,
    const aiNode*                                           node,
    bool                                                    is_root)
{
    if (!node) return;

    // The root node of the skeleton can *still* have a scene-graph parent,
    // so the is_root flag is needed, can't just check the node->mParent for null.
    if (is_root) {
        assert(joints.empty());

        const Joint root_joint = {
            .inv_bind  = glm::identity<mat4>(),
            .parent_id = Joint::no_parent,
        };

        const size_t root_joint_id = 0;

        const ResourceName joint_name = ResourceName::from_view(s2sv(node->mName));
        joints.emplace_back(root_joint);
        joint_names.emplace_back(joint_name);
        node2id.emplace(node, root_joint_id);

    } else /* not root */ {

        // "Bones" only exist for non-root nodes.
        if (const auto* mapped = try_find_value(node2bone, node)) {
            const aiBone* bone = *mapped;

            // If non-root, lookup parent id from the table.
            // The parent node should already be there because of the traversal order.
            const size_t parent_id = node2id.at(node->mParent);
            const size_t joint_id  = joints.size();

            assert(joint_id < Skeleton::max_joints); // Safety of the conversion must be guaranteed by prior importer checks.

            const Joint joint{
                .inv_bind  = m2m(bone->mOffsetMatrix),
                .parent_id = uint8_t(parent_id),
            };

            const ResourceName joint_name = ResourceName::from_view(s2sv(bone->mName));
            joints.emplace_back(joint);
            joint_names.emplace_back(joint_name);
            node2id.emplace(node, joint_id);

        } else /* no bone in this node */ {

            // If this node is not a bone, then it's something weird
            // attached to the armature and we best skip it, and its children.
            return;
        }
    }

    for (const aiNode* child : std::span(node->mChildren, node->mNumChildren)) {
        const bool is_root = false;
        populate_joints_preorder(joints, joint_names, node2id, node2bone, child, is_root);
    }
}


} // namespace


auto import_skeleton_async(
    AssetImporterContext                                    context,
    const aiNode*                                           armature,
    std::unordered_map<const aiNode*, size_t>&              node2jointid,
    const std::unordered_map<const aiNode*, const aiBone*>& node2bone)
        -> Job<UUID>
{
    co_await reschedule_to(context.thread_pool());

    std::vector<Joint>        joints;
    std::vector<ResourceName> joint_names;
    const aiNode* root    = armature;
    const bool    is_root = true;
    populate_joints_preorder(joints, joint_names, node2jointid, node2bone, root, is_root);

    const ResourcePathHint path_hint{
        .directory = "skeletons",
        .name      = s2sv(armature->mName),
        .extension = "jskel",
    };

    const SkeletonFile::Args args{
        .num_joints = uint16_t(joints.size()),
    };

    const size_t       file_size     = SkeletonFile::required_size(args);
    const ResourceType resource_type = SkeletonFile::resource_type;


    co_await reschedule_to(context.local_context());
    auto [uuid, mregion] = context.resource_database().generate_resource(resource_type, path_hint, file_size);
    co_await reschedule_to(context.thread_pool());


    SkeletonFile file = SkeletonFile::create_in(MOVE(mregion), uuid, args);

    assert(file.num_joints() == joints.size());
    assert(file.num_joints() == joint_names.size());
    const std::span dst_joints      = file.joints();
    const std::span dst_joint_names = file.joint_names();

    std::ranges::copy(joints,      dst_joints     .begin());
    std::ranges::copy(joint_names, dst_joint_names.begin());

    co_return uuid;
}


auto import_anim_async(
    AssetImporterContext                             context,
    const aiAnimation*                               ai_anim,
    const aiNode*                                    armature,
    const UUID&                                      skeleton_uuid,
    const std::unordered_map<const aiNode*, size_t>& node2jointid)
        -> Job<UUID>
{
    co_await reschedule_to(context.thread_pool());


    const double tps        = (ai_anim->mTicksPerSecond != 0) ? ai_anim->mTicksPerSecond : 30.0;
    const double duration_s = ai_anim->mDuration / tps;

    const auto    ai_joint_keyframes = std::span(ai_anim->mChannels, ai_anim->mNumChannels);
    const size_t  num_joints         = node2jointid.size();

    // Prepare the file spec first.
    std::vector<AnimationFile::KeySpec> specs(num_joints); // Zero num keys by default.
    std::vector<const aiNodeAnim*>      ai_jkfs_per_joint(num_joints, nullptr); // Joint keyframes in joint order. For later.
    for (const aiNodeAnim* ai_jkfs : ai_joint_keyframes) {

        // WHY DO I HAVE TO LOOK IT UP BY NAME JESUS.
        const aiNode* node = armature->FindNode(ai_jkfs->mNodeName);
        assert(node);
        const size_t joint_id = node2jointid.at(node);

        specs.at(joint_id) = {
            .num_pos_keys = ai_jkfs->mNumPositionKeys,
            .num_rot_keys = ai_jkfs->mNumRotationKeys,
            .num_sca_keys = ai_jkfs->mNumScalingKeys,
        };

        auto& ptr = ai_jkfs_per_joint.at(joint_id);
        assert(!ptr); // We don't expect multiple aiNodeAnims to manipulate the same joint.
        ptr = ai_jkfs;
    }

    const AnimationFile::Args args{
        .key_specs = specs,
    };

    const ResourcePathHint path_hint{
        .directory = "animations",
        .name      = s2sv(ai_anim->mName),
        .extension = "janim",
    };

    const size_t       file_size     = AnimationFile::required_size(args);
    const ResourceType resource_type = AnimationFile::resource_type;


    co_await reschedule_to(context.local_context());
    auto [uuid, mregion] = context.resource_database().generate_resource(resource_type, path_hint, file_size);
    co_await reschedule_to(context.thread_pool());


    AnimationFile file = AnimationFile::create_in(MOVE(mregion), uuid, args);

    file.skeleton_uuid() = skeleton_uuid;
    file.duration_s()    = float(duration_s);

    using ranges::views::enumerate;
    for (auto [joint_id, ai_jkfs] : enumerate(ai_jkfs_per_joint)) {
        // Could be nullptr if no keyframes exist for this joint.
        if (ai_jkfs) {
            // It is guaranteed by assimp that times are monotonically *increasing*.
            const auto ai_pos_keys = std::span(ai_jkfs->mPositionKeys, ai_jkfs->mNumPositionKeys);
            const auto ai_rot_keys = std::span(ai_jkfs->mRotationKeys, ai_jkfs->mNumRotationKeys);
            const auto ai_sca_keys = std::span(ai_jkfs->mScalingKeys,  ai_jkfs->mNumScalingKeys );

            const auto to_vec3_key = [&tps](const aiVectorKey& vk) -> AnimationFile::KeyVec3 { return { float(vk.mTime / tps), v2v(vk.mValue) }; };
            const auto to_quat_key = [&tps](const aiQuatKey&   qk) -> AnimationFile::KeyQuat { return { float(qk.mTime / tps), q2q(qk.mValue) }; };

            using std::views::transform;

            std::ranges::copy(ai_pos_keys | transform(to_vec3_key), file.pos_keys(joint_id).begin());
            std::ranges::copy(ai_rot_keys | transform(to_quat_key), file.rot_keys(joint_id).begin());
            std::ranges::copy(ai_sca_keys | transform(to_vec3_key), file.sca_keys(joint_id).begin());
        }
    }

    co_return uuid;
}


} // namespace josh::detail
