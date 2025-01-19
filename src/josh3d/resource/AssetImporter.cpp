#include "AssetImporter.hpp"
#include "Asset.hpp"
#include "CategoryCasts.hpp"
#include "Channels.hpp"
#include "Coroutines.hpp"
#include "Ranges.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceFiles.hpp"
#include "Skeleton.hpp"
#include "TaskCounterGuard.hpp"
#include "TextureHelpers.hpp"
#include "Transform.hpp"
#include "UUID.hpp"
#include "VertexPNUTB.hpp"
#include <assimp/BaseImporter.h>
#include <assimp/Importer.hpp>
#include <assimp/anim.h>
#include <assimp/matrix4x4.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <assimp/vector3.h>
#include <filesystem>
#include <jsoncons/basic_json.hpp>
#include <jsoncons/json_options.hpp>
#include <jsoncons/tag_type.hpp>
#include <range/v3/iterator/operations.hpp>
#include <jsoncons/json.hpp>
#include <ranges>
#include <unordered_map>


namespace josh {


AssetImporter::AssetImporter(
    ResourceDatabase&  resource_database,
    ThreadPool&        loading_pool,
    OffscreenContext&  offscreen_context,
    CompletionContext& completion_context
)
    : resource_database_ { resource_database  }
    , thread_pool_       { loading_pool       }
    , offscreen_context_ { offscreen_context  }
    , completion_context_{ completion_context }
{}


void AssetImporter::update() {
    while (std::optional task = local_context_.tasks.try_pop()) {
        (*task)();
    }
}


class AssetImporter::Access {
public:
    auto& resource_database()  noexcept { return self_.resource_database_;  }
    auto& thread_pool()        noexcept { return self_.thread_pool_;        }
    auto& offscreen_context()  noexcept { return self_.offscreen_context_;  }
    auto& completion_context() noexcept { return self_.completion_context_; }
    auto& task_counter()       noexcept { return self_.task_counter_;       }
    auto& local_context()      noexcept { return self_.local_context_;      }
private:
    friend AssetImporter;
    Access(AssetImporter& self) : self_{ self } {}
    AssetImporter& self_;
};


namespace {


using TextureIndex    = int32_t;
using MaterialIndex   = size_t;
using TextureJobIndex = size_t;


struct TextureInfo {
    TextureIndex id     = -1;
    ImageIntent  intent = ImageIntent::Unknown;
};


struct MaterialIDs {
    TextureIndex diffuse_id  = -1;
    TextureIndex specular_id = -1;
    TextureIndex normal_id   = -1;
};


struct MaterialUUIDs {
    UUID diffuse_uuid;
    UUID specular_uuid;
    UUID normal_uuid;
};


auto v2v(const aiVector3D& v) noexcept
    -> vec3
{
    return { v.x, v.y, v.z };
}


auto q2q(const aiQuaternion& q) noexcept
    -> quat
{
    return { q.w, q.x, q.y, q.z };
}


auto m2m(const aiMatrix4x4& m) noexcept
    -> mat4
{
    // From assimp docs:
    //
    // "The transposition has nothing to do with a left-handed or right-handed
    // coordinate system  but 'converts' between row-major and column-major storage formats."
    auto msrc = m;
    msrc.Transpose();

    mat4 mdst;
    for (size_t i{ 0 }; i < 4; ++i) {
        for (size_t j{ 0 }; j < 4; ++j) {
            mdst[int(i)][int(j)] = msrc[i][j];
        }
    }

    return mdst;
}

// Decomposes the assimp Matrix4x4 into a regular transform.
auto m2tf(const aiMatrix4x4& m) noexcept
    -> Transform
{
    aiVector3D   pos;
    aiQuaternion rot;
    aiVector3D   sca;
    m.Decompose(sca, rot, pos);
    return { v2v(pos), q2q(rot), v2v(sca) };
}


auto s2sv(const aiString& s) noexcept
    -> std::string_view
{
    return { s.data, s.length };
}


[[nodiscard]]
auto import_texture_async(
    AssetImporter::Access importer,
    Path                  src_filepath,
    ImportTextureParams   params)
        -> Job<UUID>
{
    const auto task_guard = importer.task_counter().obtain_task_guard();
    co_await reschedule_to(importer.thread_pool());


    auto data   = load_image_data_from_file<chan::UByte>(File(src_filepath), 4, 4);
    auto [w, h] = data.resolution();


    const Path name = src_filepath.stem();
    const ResourcePathHint path_hint{
        .directory = "textures",
        .name      = name.c_str(),
        .extension = "jtxtr",
    };

    using StorageFormat = TextureFile::StorageFormat;
    using MIPSpec       = TextureFile::MIPSpec;

    MIPSpec spec[1]{
        MIPSpec{
            .size_bytes    = uint32_t(data.size_bytes()),
            .width_pixels  = uint16_t(w),
            .height_pixels = uint16_t(h),
        }
    };

    assert(params.storage_format == StorageFormat::RAW); // Everything else is not supported yet.

    const TextureFile::Args args{
        .format    = params.storage_format,
        .mip_specs = spec,
    };

    const size_t file_size = TextureFile::required_size(args);


    co_await reschedule_to(importer.local_context());
    auto [uuid, mapped_region] = importer.resource_database().generate_resource(path_hint, file_size);
    co_await reschedule_to(importer.thread_pool());


    TextureFile file = TextureFile::create_in(MOVE(mapped_region), args);

    const std::span<std::byte> dst_bytes = file.mip_bytes(0);

    using std::views::transform;
    // TODO: Another reason to drop the usage of std::byte. It's dumb and makes you write amazing code like:
    std::ranges::copy(data | transform([](chan::UByte b) { return std::byte(b); }), dst_bytes.begin());

    co_return uuid;
}


void populate_joints_preorder(
    std::vector<Joint>&                                     joints,
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

        joints.emplace_back(root_joint);
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

            joints.emplace_back(joint);
            node2id.emplace(node, joint_id);

        } else /* no bone in this node */ {

            // If this node is not a bone, then it's something weird
            // attached to the armature and we best skip it, and its children.
            return;
        }
    }

    for (const aiNode* child : std::span(node->mChildren, node->mNumChildren)) {
        const bool is_root = false;
        populate_joints_preorder(joints, node2id, node2bone, child, is_root);
    }
}



[[nodiscard]]
auto import_skeleton_async(
    AssetImporter::Access                                   importer,
    const aiNode*                                           armature,
    std::unordered_map<const aiNode*, size_t>&              node2jointid,
    const std::unordered_map<const aiNode*, const aiBone*>& node2bone)
        -> Job<UUID>
{
    const auto task_guard = importer.task_counter().obtain_task_guard();
    co_await reschedule_to(importer.thread_pool());

    std::vector<Joint> joints;
    const aiNode* root    = armature;
    const bool    is_root = true;
    populate_joints_preorder(joints, node2jointid, node2bone, root, is_root);

    const ResourcePathHint path_hint{
        .directory = "skeletons",
        .name      = s2sv(armature->mName),
        .extension = "jskel",
    };

    const SkeletonFile::Args args{
        .num_joints = uint16_t(joints.size()),
    };

    const size_t file_size = SkeletonFile::required_size(args);


    co_await reschedule_to(importer.local_context());
    auto [uuid, mregion] = importer.resource_database().generate_resource(path_hint, file_size);
    co_await reschedule_to(importer.thread_pool());


    SkeletonFile file = SkeletonFile::create_in(MOVE(mregion), args);

    // TODO: Joint names.
    assert(file.num_joints() == joints.size());
    const std::span<Joint> dst_joints = file.joints();

    std::ranges::copy(joints, dst_joints.begin());

    co_return uuid;
}


void extract_skinned_mesh_verts_to(
    std::span<VertexSkinned>                         out_verts,
    const aiMesh*                                    ai_mesh,
    const std::unordered_map<const aiNode*, size_t>& node2jointid)
{
    auto positions  = std::span(ai_mesh->mVertices,         ai_mesh->mNumVertices);
    auto uvs        = std::span(ai_mesh->mTextureCoords[0], ai_mesh->mNumVertices);
    auto normals    = std::span(ai_mesh->mNormals,          ai_mesh->mNumVertices);
    auto tangents   = std::span(ai_mesh->mTangents,         ai_mesh->mNumVertices);
    auto bones      = std::span(ai_mesh->mBones,            ai_mesh->mNumBones   );

    assert(out_verts.size() == positions.size());

    const bool valid_num_bones = bones.size() <= Skeleton::max_joints;

    if (!normals.data())    { throw error::AssetContentsParsingError("Mesh data does not contain Normals.");    }
    if (!uvs.data())        { throw error::AssetContentsParsingError("Mesh data does not contain UVs.");        }
    if (!tangents.data())   { throw error::AssetContentsParsingError("Mesh data does not contain Tangents.");   }
    if (!bones.data())      { throw error::AssetContentsParsingError("Mesh data does not contain Bones.");      }
    if (!valid_num_bones)   { throw error::AssetContentsParsingError("Armature has too many Bones (>255)."); }


    using glm::uvec4;
    // Info about weights as pulled from assimp,
    // before convertion to a more "strict" packed internal format.
    struct VertJointInfo {
        vec4   ws {}; // Uncompressed weights.
        uvec4  ids{}; // Refer to root node by default.
        int8_t n  {}; // Variable number of weights+ids. Because 4 is only an upper limit.
    };

    std::vector<VertJointInfo> vert_joint_infos;
    vert_joint_infos.resize(positions.size()); // Resize, not reserve.

    // Now fill out the ids and weights for each vertex.
    for (const aiBone* bone : bones) {
        for (const aiVertexWeight& w : std::span(bone->mWeights, bone->mNumWeights)) {
            auto& info = vert_joint_infos[w.mVertexId];
            info.ws [info.n] = w.mWeight;
            info.ids[info.n] = node2jointid.at(bone->mNode);
            ++info.n;
            assert(info.n <= 4);
        }
    }

    for (size_t i{ 0 }; i < positions.size(); ++i) {
        const auto& joint_info = vert_joint_infos[i];

        out_verts[i] = VertexSkinned::pack(
            v2v(positions[i]),
            v2v(uvs      [i]),
            v2v(normals  [i]),
            v2v(tangents [i]),
            joint_info.ids,
            joint_info.ws
        );
    }
}


void extract_static_mesh_verts_to(
    std::span<VertexPNUTB> out_verts,
    const aiMesh*          ai_mesh)
{
    auto positions  = std::span(ai_mesh->mVertices,         ai_mesh->mNumVertices);
    auto uvs        = std::span(ai_mesh->mTextureCoords[0], ai_mesh->mNumVertices);
    auto normals    = std::span(ai_mesh->mNormals,          ai_mesh->mNumVertices);
    auto tangents   = std::span(ai_mesh->mTangents,         ai_mesh->mNumVertices);
    auto bitangents = std::span(ai_mesh->mBitangents,       ai_mesh->mNumVertices);

    if (!normals.data())    { throw error::AssetContentsParsingError("Mesh data does not contain Normals.");    }
    if (!uvs.data())        { throw error::AssetContentsParsingError("Mesh data does not contain UVs.");        }
    if (!tangents.data())   { throw error::AssetContentsParsingError("Mesh data does not contain Tangents.");   }
    if (!bitangents.data()) { throw error::AssetContentsParsingError("Mesh data does not contain Bitangents."); }

    assert(out_verts.size() == positions.size());

    for (size_t i{ 0 }; i < out_verts.size(); ++i) {
        out_verts[i] = VertexPNUTB{
            .position  = v2v(positions [i]),
            .normal    = v2v(normals   [i]),
            .uv        = v2v(uvs       [i]),
            .tangent   = v2v(tangents  [i]),
            .bitangent = v2v(bitangents[i]),
        };
    }
}


void extract_mesh_elems_to(
    std::span<uint32_t> out_elems,
    const aiMesh*       ai_mesh)
{
    auto faces = std::span(ai_mesh->mFaces, ai_mesh->mNumFaces);

    assert(out_elems.size() == faces.size() * 3);

    for (size_t f{ 0 }; f < faces.size(); ++f) {
        const size_t  e    = f * 3; // Index of the first element in the face.
        const aiFace& face = faces[f];
        assert(face.mNumIndices == 3); // Must be guaranteed by the aiProcess_Triangulate flag.
        out_elems[e + 0] = face.mIndices[0];
        out_elems[e + 1] = face.mIndices[1];
        out_elems[e + 2] = face.mIndices[2];
    }
}


[[nodiscard]]
auto import_mesh_async(
    AssetImporter::Access                            importer,
    const aiMesh*                                    ai_mesh,
    UUID                                             skeleton_uuid,
    const std::unordered_map<const aiNode*, size_t>* node2jointid) // Optional, if Skinned.
        -> Job<UUID>
{
    const auto task_guard = importer.task_counter().obtain_task_guard();
    co_await reschedule_to(importer.thread_pool());

    const ResourcePathHint path_hint{
        .directory = "meshes",
        .name      = s2sv(ai_mesh->mName),
        .extension = "jmesh",
    };

    using VertexLayout = MeshFile::VertexLayout;
    using LODSpec      = MeshFile::LODSpec;

    LODSpec spec[1]{
        LODSpec{
            .num_verts = ai_mesh->mNumVertices,
            .num_elems = ai_mesh->mNumFaces * 3,
        },
    };

    const VertexLayout layout = [&]{
        if (ai_mesh->HasBones()) {
            return VertexLayout::Skinned;
        } else {
            return VertexLayout::Static;
        }
    }();

    const MeshFile::Args args{
        .layout    = layout,
        .lod_specs = spec,
    };

    const size_t file_size = MeshFile::required_size(args);


    co_await reschedule_to(importer.local_context());
    auto [uuid, mregion] = importer.resource_database().generate_resource(path_hint, file_size);
    co_await reschedule_to(importer.thread_pool());



    MeshFile file = MeshFile::create_in(MOVE(mregion), args);

    file.skeleton_uuid() = skeleton_uuid;

    if (layout == VertexLayout::Skinned) {
        std::span<VertexSkinned> dst_verts = file.lod_verts<VertexLayout::Skinned>(0);
        assert(node2jointid);
        extract_skinned_mesh_verts_to(dst_verts, ai_mesh, *node2jointid);
    } else if (layout == VertexLayout::Static) {
        std::span<VertexPNUTB> dst_verts = file.lod_verts<VertexLayout::Static>(0);
        extract_static_mesh_verts_to(dst_verts, ai_mesh);
    }

    std::span<uint32_t> dst_elems = file.lod_elems(0);
    extract_mesh_elems_to(dst_elems, ai_mesh);


    co_return uuid;
}


[[nodiscard]]
auto import_mesh_desc_async(
    AssetImporter::Access importer,
    UUID                  mesh_uuid,
    std::string_view      name,
    MaterialUUIDs         mat_uuids)
        -> Job<UUID>
{
    /*
    Simple json spec for the time being:

    {
        "mesh": "f3f2e850-b5d4-11ef-ac7e-96584d5248b2",
        "diffuse": "1d07af07-eafc-48e5-a618-30722b576dc6",
        "normal":  "1d07af07-eafc-48e5-a618-30722b576dc6",
        "specular": "1d07af07-eafc-48e5-a618-30722b576dc6",
        "specpower": 128.0
    }

    */
    // We will construct json as text first, serialize to a string,
    // then request the resource file from the database at a later point.
    using jsoncons::json;

    json j;
    j["mesh"]      = serialize_uuid(mesh_uuid);
    j["diffuse"]   = serialize_uuid(mat_uuids.diffuse_uuid);
    j["normal"]    = serialize_uuid(mat_uuids.normal_uuid);
    j["specular"]  = serialize_uuid(mat_uuids.specular_uuid);
    j["specpower"] = 128.f;
    std::string json_string;
    j.dump(json_string, jsoncons::indenting::indent);

    const ResourcePathHint path_hint{
        .directory = "meshes",
        .name      = name,
        .extension = "jmdesc",
    };


    // After writing json to string (and learning the required size),
    // we go back to the resource database to generate actual files.
    co_await reschedule_to(importer.local_context());
    auto [uuid, mregion] = importer.resource_database().generate_resource(path_hint, json_string.size());
    co_await reschedule_to(importer.thread_pool());


    // Finally, we can write the contents of the files through the mapped region.
    auto dst_bytes = std::span((std::byte*)mregion.get_address(), mregion.get_size());
    auto src_bytes = std::as_bytes(std::span(json_string));
    assert(src_bytes.size() == dst_bytes.size());
    std::ranges::copy(src_bytes, dst_bytes.begin());

    co_return uuid;
}


[[nodiscard]]
auto import_anim_async(
    AssetImporter::Access                            importer,
    const aiAnimation*                               ai_anim,
    const aiNode*                                    armature,
    const UUID&                                      skeleton_uuid,
    const std::unordered_map<const aiNode*, size_t>& node2jointid)
        -> Job<UUID>
{
    const auto task_guard = importer.task_counter().obtain_task_guard();
    co_await reschedule_to(importer.thread_pool());

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

    const size_t file_size = AnimationFile::required_size(args);


    co_await reschedule_to(importer.local_context());
    auto [uuid, mregion] = importer.resource_database().generate_resource(path_hint, file_size);
    co_await reschedule_to(importer.thread_pool());


    AnimationFile file = AnimationFile::create_in(MOVE(mregion), args);

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


auto get_path_to_ai_texture(
    const Path&       parent_dir,
    const aiMaterial* material,
    aiTextureType     type)
        -> Path
{
    aiString filename;
    material->GetTexture(type, 0, &filename);
    return parent_dir / filename.C_Str();
};


auto get_ai_texture_type(const Path& path, ImageIntent intent)
    -> aiTextureType
{
    switch (intent) {
        case ImageIntent::Albedo:
            return aiTextureType_DIFFUSE;
        case ImageIntent::Specular:
            return aiTextureType_SPECULAR;
        case ImageIntent::Normal:
            // FIXME: Surely there's a better way.
            if (Assimp::BaseImporter::SimpleExtensionCheck(path, "obj")) {
                return aiTextureType_HEIGHT;
            } else {
                return aiTextureType_NORMALS;
            }
        case ImageIntent::Alpha:
            return aiTextureType_OPACITY;
        case ImageIntent::Heightmap:
            return aiTextureType_DISPLACEMENT;
        case ImageIntent::Unknown:
            return aiTextureType_UNKNOWN;
        default:
            assert(false); // ???
            return aiTextureType_UNKNOWN;
    }
};


void populate_scene_nodes_preorder(
    jsoncons::json&                                         array,
    const aiNode*                                           node,
    std::unordered_map<const aiNode*, size_t>&              node2sceneid,
    std::unordered_map<size_t, std::vector<const aiNode*>>& meshid2nodes,
    const std::unordered_map<const aiNode*, const aiBone*>& node2bone)
{
    if (!node) return;

    // We do not populate the actual entry data as each node does not
    // directly reference the type of entity it represents.
    //
    // Instead, we do only the following:
    //
    //  - Populate each node with scene graph information: "parent", "transform" and "name".
    //    We skip the bone nodes here though, as we have no way to deal with it.
    //
    //  - Build a map from node ptr to an index in the `array`, so that later
    //    processing can reference the right array element from the node ptr
    //    and emplace there the relevant components.
    //
    //  - Populate a map from each mesh to a set of nodes that reference it.
    //

    const size_t scene_id = array.size();

    // If bone, stop traversal here. Skeletons aren't part of the scene graph in our model.
    // NOTE: We miss out on the information about nodes attached to joints, but since
    // we have no way of representing that either, it's no big deal so far.
    if (node2bone.contains(node)) return;

    auto [it, was_emplaced] = node2sceneid.emplace(node, scene_id);
    assert(was_emplaced);

    if (node->mNumMeshes) {
        if (node->mNumMeshes > 1) {
            // TODO: This insane. Why is this even possible? Best we could do is to create a common parent.
            // The issue is that then our graph and assimp graph will not be isomorphic.
            throw error::AssetContentsParsingError("Single nodes with multiple meshes are not supported.");
        }

        const auto ai_mesh_id = node->mMeshes[0];
        meshid2nodes[ai_mesh_id].push_back(node);
    }


    assert(array.is_array());
    jsoncons::json j;

    if (node->mName.length) {
        j["name"] = s2sv(node->mName);
    }

    const Transform tf = m2tf(node->mTransformation);

    auto transform_as_json = [](const Transform& tf) {
        jsoncons::json j;
        const vec3& pos = tf.position();
        const quat& rot = tf.orientation();
        const vec3& sca = tf.scaling();
        j["position"] = jsoncons::json(jsoncons::json_array_arg, { pos.x, pos.y, pos.z });
        j["rotation"] = jsoncons::json(jsoncons::json_array_arg, { rot.w, rot.x, rot.y, rot.z });
        j["scaling"]  = jsoncons::json(jsoncons::json_array_arg, { sca.x, sca.y, sca.z });
        return j;
    };

    j["transform"] = transform_as_json(tf);

    if (node->mParent) {
        const auto* parent_id = try_find_value(node2sceneid, node->mParent);
        // `node2sceneid` is populated in pre-order, so we should always find our parent there.
        assert(parent_id);
        j["parent"] = *parent_id;
    }

    array.push_back(MOVE(j));


    for (const aiNode* child : std::span(node->mChildren, node->mNumChildren)) {
        populate_scene_nodes_preorder(array, child, node2sceneid, meshid2nodes, node2bone);
    }
}


[[nodiscard]]
auto import_model_async(
    AssetImporter::Access importer,
    Path                  path,
    ImportModelParams     params)
        -> Job<UUID>
{
    const auto task_guard = importer.task_counter().obtain_task_guard();
    co_await reschedule_to(importer.thread_pool());

    const Path parent_dir = path.parent_path(); // Reused in a few places.

    Assimp::Importer ai_importer;

    // Some flags are hardcoded, the following processing
    // relies on some of these flags being always set.
    constexpr auto base_flags =
        aiProcess_Triangulate              | // Required.
        aiProcess_GenUVCoords              | // Required. Uhh, how does assimp generate this?
        aiProcess_GenSmoothNormals         | // Required.
        aiProcess_CalcTangentSpace         | // Required.
        aiProcess_LimitBoneWeights         | // Required. Up to 4 weights with most effect.
        aiProcess_PopulateArmatureData     | // Required. Figures out which skeletons are referenced by which mesh.
        aiProcess_GenBoundingBoxes         | // Required.
        aiProcess_GlobalScale              | // TODO: What does this do exactly?
        aiProcess_RemoveRedundantMaterials | // Does not destroy any information. Keep default.
        aiProcess_ImproveCacheLocality;      // Does not destroy any information. Keep default.

    const auto extra_flags =
        (params.collapse_graph ? aiProcess_OptimizeGraph  : 0) | // Destructive. Leave as an option only.
        (params.merge_meshes   ? aiProcess_OptimizeMeshes : 0);  // Very aggressive, but perf gains can be substantial.

    const auto flags = base_flags | extra_flags;

    const aiScene* ai_scene = ai_importer.ReadFile(path, flags);

    if (!ai_scene) { throw error::AssetFileImportFailure(path, ai_importer.GetErrorString()); }

    const auto ai_meshes    = std::span(ai_scene->mMeshes,     ai_scene->mNumMeshes   ); // Order: Meshes.
    const auto ai_materials = std::span(ai_scene->mMaterials,  ai_scene->mNumMaterials);
    const auto ai_anims     = std::span(ai_scene->mAnimations, ai_scene->mNumAnimations);


    const auto get_job_result = [](auto&& job) { return job.get_result(); };


    // Texture loads are independent of anything else, they also are
    // the only resource that actually has to load extra data from disk.
    // So we launch texture jobs as early as possible, anticipating
    // that loading them will take the longest anyway.

    std::unordered_map<const aiMaterial*, MaterialIDs> material2matids;
    std::unordered_map<Path, TextureInfo>              path2texinfo;

    {
        // Will be used to assign new indices for textures. These are global for all textures in all materials.
        TextureIndex next_texture_index{ 0 };

        auto assign_texture_index = [&](const aiMaterial* ai_material, ImageIntent intent)
            -> TextureIndex
        {
            const aiTextureType ai_type = get_ai_texture_type(path, intent);
            const bool          exists  = ai_material->GetTextureCount(ai_type);

            if (!exists) { return -1; } // If no texture corresponding to this ImageIntent in the material.

            Path texture_path = get_path_to_ai_texture(parent_dir, ai_material, ai_type);

            const TextureInfo texture_info{
                .id     = next_texture_index,
                .intent = intent,
            };

            auto [it, was_emplaced] = path2texinfo.emplace(MOVE(texture_path), texture_info);

            if (was_emplaced) { ++next_texture_index; }

            // If it wasn't emplaced, then it was already there,
            // Either way, we can get the index from `it`.
            return it->second.id;
        };

        material2matids.reserve(ai_materials.size());
        for (const aiMaterial* ai_material : ai_materials) {
            const MaterialIDs matids{
                .diffuse_id  = assign_texture_index(ai_material, ImageIntent::Albedo),
                .specular_id = assign_texture_index(ai_material, ImageIntent::Specular),
                .normal_id   = assign_texture_index(ai_material, ImageIntent::Normal),
            };
            material2matids.emplace(ai_material, matids);
        }
    }

    // Now we have a set of texture paths that we need to load.
    // We'll submit jobs for them and then move on to loading other stuff.
    const size_t num_textures = path2texinfo.size();

    std::vector<Job<UUID>>       texture_jobs;
    std::vector<TextureJobIndex> texid2jobid;
    texid2jobid .resize (num_textures);
    texture_jobs.reserve(num_textures);

    for (const auto& item : path2texinfo) {
        const auto& [path, tex_info] = item;
        texid2jobid[tex_info.id] = texture_jobs.size();
        // FIXME: This needs to be resolved better.
        const ImportTextureParams params{
            .storage_format = TextureFile::StorageFormat::RAW,
        };
        texture_jobs.emplace_back(import_texture_async(importer, path, params));
    }


    // Meshes and Animations depend on the Skeleton UUIDs, so do them before.
    //
    // Before loading skeletons, however, we need some extra information
    // about bones and nodes. Prepopulate it here.
    //
    // NOTE: "Armature" is a node that uniquely describes a particular skeleton,
    // we use it as the skeleton identity.

    // FIXME: The way we do this, we won't import skeletons if they have
    // no meshes referencing them in the file. This is not nice.

    std::unordered_map<const aiNode*, const aiBone*>                   node2bone;
    std::unordered_map<const aiMesh*, const aiNode*>                   mesh2armature;
    std::unordered_map<const aiNode*, std::vector<const aiAnimation*>> armature2anims;
    std::unordered_map<const aiAnimation*, const aiNode*>              anim2armature;
    std::unordered_set<const aiNode*>                                  armatures; // Order: Skeleton.

    for (const aiMesh* ai_mesh : ai_meshes) {
        if (ai_mesh->HasBones()) {
            const auto bones = std::span(ai_mesh->mBones, ai_mesh->mNumBones);

            // Populate node2bone for all bones of this mesh.
            for (const aiBone* bone : bones) {
                node2bone.try_emplace(bone->mNode, bone);
            }

            // Populate associated armatures for each skinned mesh.
            assert(bones.size());
            const aiNode* armature = bones[0]->mArmature;
            mesh2armature.emplace(ai_mesh, armature);
            armatures.emplace(armature);

            // Figure out which animation belongs to which skeleton.
            //
            // NOTE: This is not going to work if the animation manipulates both
            // the skeleton joints and scene-graph nodes. For that, we'd
            // need to build a set of keyed nodes and do a set-on-set intersection tests.
            // We don't bother currently, since we can't even represent such "mixed" animation.
            for (const aiAnimation* ai_anim : ai_anims) {
                const auto channels = std::span(ai_anim->mChannels, ai_anim->mNumChannels);
                assert(channels.size()); // Animation with 0 keyframes? Is that even possible?
                const aiNode* affected_node = armature->FindNode(channels[0]->mNodeName);
                if (affected_node) {
                    armature2anims[armature].emplace_back(ai_anim);
                    anim2armature.emplace(ai_anim, armature);
                }
            }
        }
    }

    // Before we can convert all animations and meshes to our format,
    // we'll need all skeletons to be created with their UUID established,
    // since each animation and each mesh must reference a common skeleton.
    std::vector<Job<UUID>> skeleton_jobs; // Order: Skeletons.

    using Node2JointID = std::unordered_map<const aiNode*, size_t>;
    // Maps: Bone Node -> Joint ID per armature. The name is ridiculous.
    // Populated inside populate_joints_preorder() as the order is established.
    std::unordered_map<const aiNode*, Node2JointID> armature2_node2jointid;

    skeleton_jobs         .reserve(armatures.size());
    armature2_node2jointid.reserve(armatures.size());

    // Submit skeleton jobs. This will also populate the respective entries in node2jointids.
    for (const aiNode* armature : armatures) {
        // Emplace empty. Populate when importing.
        auto [it, was_emplaced] = armature2_node2jointid.try_emplace(armature);
        assert(was_emplaced);
        Node2JointID& node2jointid = it->second;
        skeleton_jobs.emplace_back(import_skeleton_async(importer, armature, node2jointid, node2bone));
    }

    co_await importer.completion_context().until_all_ready(skeleton_jobs);
    co_await reschedule_to(importer.thread_pool());


    // Now unpack the relationship betweed each armature and associated UUID.
    std::vector<UUID> skeleton_uuids = // Order: Skeletons.
        skeleton_jobs | transform(get_job_result) | ranges::to<std::vector>();

    skeleton_jobs.clear();

    std::unordered_map<const aiNode*, UUID> armature2uuid =
        zip(armatures, skeleton_uuids) | ranges::to<std::unordered_map>();



    // Finally, we can submit importing of Meshes and Animations,
    // so that they can reference correct Skeletons.
    //
    // NOTE: This is technically not required to be ordered like
    // this if we permit "patching" referenced skeletons in the
    // mesh and animation files after creating the files.
    // That would probably be better from task scheduling perspective
    // and performance, but the current way is just simpler.

    std::vector<Job<UUID>> mesh_jobs; // Order: Meshes.
    std::vector<Job<UUID>> anim_jobs; // Order: Anims.

    mesh_jobs.reserve(ai_meshes.size());
    anim_jobs.reserve(ai_anims.size());

    for (const aiMesh* ai_mesh : ai_meshes) {
        UUID                skeleton_uuid = {};
        const Node2JointID* node2jointid  = nullptr;

        if (const auto* item = try_find(mesh2armature, ai_mesh)) {
            const auto [mesh, armature] = *item;
            skeleton_uuid = armature2uuid.at(armature);
            node2jointid  = &armature2_node2jointid.at(armature);
        }

        mesh_jobs.emplace_back(import_mesh_async(importer, ai_mesh, skeleton_uuid, node2jointid));
    }

    for (const aiAnimation* ai_anim : ai_anims) {
        const aiNode* armature           = anim2armature.at(ai_anim);
        const UUID skeleton_uuid         = armature2uuid.at(armature);
        const Node2JointID& node2jointid = armature2_node2jointid.at(armature);
        anim_jobs.emplace_back(import_anim_async(importer, ai_anim, armature, skeleton_uuid, node2jointid));
    }


    // Wait for completion of mesh data and texture jobs, so that
    // we could assemble the mesh description files.

    co_await importer.completion_context().until_all_ready(mesh_jobs);
    co_await importer.completion_context().until_all_ready(texture_jobs);
    co_await reschedule_to(importer.thread_pool());


    std::vector<UUID> texture_uuids = texture_jobs | transform(get_job_result) | ranges::to<std::vector>();
    std::vector<UUID> mesh_uuids    = mesh_jobs    | transform(get_job_result) | ranges::to<std::vector>(); // Order: Meshes.


    // "Mesh Description" is a file that just references a Mesh+Material.
    // TODO: We should probably have a "Material" file too.

    std::vector<Job<UUID>> mdesc_jobs; // Order: Meshes.

    mdesc_jobs.reserve(mesh_uuids.size());

    for (auto [ai_mesh, mesh_uuid] : zip(ai_meshes, mesh_uuids)) {

        const auto get_uuid_from_texid = [&](TextureIndex id) -> UUID {
            return id >= 0 ? texture_uuids[texid2jobid.at(id)] : UUID{};
        };

        const MaterialIDs&   mat = material2matids.at(ai_materials[ai_mesh->mMaterialIndex]);
        const MaterialUUIDs& mat_uuids{
            .diffuse_uuid  = get_uuid_from_texid(mat.diffuse_id),
            .specular_uuid = get_uuid_from_texid(mat.specular_id),
            .normal_uuid   = get_uuid_from_texid(mat.normal_id),
        };

        mdesc_jobs.emplace_back(import_mesh_desc_async(importer, mesh_uuid, s2sv(ai_mesh->mName), mat_uuids));
    }



    co_await importer.completion_context().until_all_ready(anim_jobs);
    co_await importer.completion_context().until_all_ready(mdesc_jobs);
    co_await reschedule_to(importer.thread_pool());


    std::vector<UUID> mdesc_uuids = mdesc_jobs | transform(get_job_result) | ranges::to<std::vector>(); // Order: Meshes.

    // Assemble the final model file, which references all imported assets,
    // and stores the final scene graph.

    /*

    "Scene" is all the stuff that has been imported.
    We currently don't import lights of cameras, but
    if could be considered too, as that's not too
    difficult.

    This is a flat array of heterogeneous objects
    with relationships between them forming a scene graph.

    Very similar to what we had in the SceneImporter, except
    that resources are referenced by their UUID, and
    the scene graph is encoded by parent id.

    */

    // Iterate through the scene in pre-order, this gives us an opportunity to
    // map children to parents in-place. We also emplace transforms and names.
    using jsoncons::json;
    json entities_array = json(jsoncons::json_array_arg);
    std::unordered_map<const aiNode*, size_t>              node2sceneid;
    std::unordered_map<size_t, std::vector<const aiNode*>> meshid2nodes; // Index in ai_meshes -> aiNodes

    populate_scene_nodes_preorder(
        entities_array,
        ai_scene->mRootNode,
        node2sceneid,
        meshid2nodes,
        node2bone
    );

    // Handle each entity type: Meshes, Lights, Cameras.
    //
    // NOTE: I really hope that the same aiNode cannot be referenced by multiple entities at once.
    // That is, if, for example, a single node referenced both a Mesh and a Camera.
    // That would be completely unhinged and break many assumptions we have.
    // Assimp, please, be sane for once.

    // NOTE: Meshes are found by references in the graph, since more than one
    // node can reference the same mesh (instancing).
    // Meshes *cannot* be found by name and their names are not even required to exist.
    for (const size_t i : irange(ai_meshes.size())) {
        const UUID&   mdesc_uuid = mdesc_uuids[i];
        const auto    nodes      = std::span(meshid2nodes.at(i));
        for (const aiNode* node : nodes) {
            // Lookup the array entry in the scene array and add the mesh component info.
            const size_t scene_id = node2sceneid.at(node);
            auto& e = entities_array[scene_id];
            e["type"]       = "Mesh";
            e["mdesc_uuid"] = serialize_uuid(mdesc_uuid);
        }
    }

    // NOTE: Lights are found by name lookup.
    for (const aiLight* ai_light : std::span(ai_scene->mLights, ai_scene->mNumLights)) {
        const aiNode* node = ai_scene->mRootNode->FindNode(ai_light->mName);
        const size_t scene_id = node2sceneid.at(node);
        auto& e = entities_array[scene_id];
        // TODO: Actually fill out the fields?

    }

    // NOTE: Cameras are found by name lookup.
    for (const aiCamera* ai_camera : std::span(ai_scene->mCameras, ai_scene->mNumCameras)) {
        // TODO:
        // ai_camera->mName;
    }

    // RANT: Assimp cannot decide whether it wants to do lookup "by-name", "by-pointer" or "by-index".
    // Guuys, can you just pick one and stick with it? No? Okaaay ;_;


    json scene_json;
    scene_json["entities"] = MOVE(entities_array);
    std::string scene_json_string;
    scene_json.dump(scene_json_string, jsoncons::indenting::indent);

    const ResourcePathHint path_hint{
        .directory = "scenes",
        .name      = s2sv(ai_scene->mName),
        .extension = "jscene",
    };


    co_await reschedule_to(importer.local_context());
    auto [uuid, mregion] = importer.resource_database().generate_resource(path_hint, scene_json_string.size());
    co_await reschedule_to(importer.thread_pool());


    // Write the scene info to the file.
    {
        auto dst_bytes = std::span((std::byte*)mregion.get_address(), mregion.get_size());
        auto src_bytes = std::as_bytes(std::span(scene_json_string));
        assert(src_bytes.size() == dst_bytes.size());
        std::ranges::copy(src_bytes, dst_bytes.begin());
    }

    // TODO: Each job can fail. How to handle?
    // Use remove_resource_later()?

    co_return uuid;
}


} // namespace


auto AssetImporter::import_texture(Path src_filepath, ImportTextureParams params)
    -> Job<UUID>
{
    return import_texture_async({ *this }, MOVE(src_filepath), params);
}


auto AssetImporter::import_model(Path path, ImportModelParams params)
    -> Job<UUID>
{
    return import_model_async({ *this }, MOVE(path), params);
}


} // namespace josh
