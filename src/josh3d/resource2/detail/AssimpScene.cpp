#include "AssimpCommon.hpp"
#include "Asset.hpp"
#include "AssetImporter.hpp"
#include "Common.hpp"
#include "ContainerUtils.hpp"
#include "Filesystem.hpp"
#include "Ranges.hpp"
#include "ResourceFiles.hpp"
#include <assimp/camera.h>
#include <assimp/light.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/BaseImporter.h>
#include <assimp/Importer.hpp>
#include <jsoncons/json.hpp>


namespace josh::detail {
namespace {


auto get_path_to_ai_texture(
    const Path&       parent_dir,
    const aiMaterial* material,
    aiTextureType     type)
        -> Path
{
    aiString filename;
    material->GetTexture(type, 0, &filename);
    return parent_dir / filename.C_Str();
}


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
}


void populate_scene_nodes_preorder(
    jsoncons::json&                              array,
    const aiScene*                               ai_scene,
    const aiNode*                                node,
    HashMap<const aiNode*, size_t>&              node2sceneid,
    std::unordered_multimap<size_t, size_t>&     meshid2sceneids,
    const HashMap<const aiNode*, const aiBone*>& node2bone)
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

    // If bone, stop traversal here. Skeleton joints aren't part of the scene graph in our model.
    // NOTE: We miss out on the information about nodes attached to joints, but since
    // we have no way of representing that either, it's no big deal so far.
    if (node2bone.contains(node)) return;


    assert(array.is_array());
    const size_t primary_scene_id = array.size(); // Not accounting for multimesh leaves.
    array.emplace_back();

    auto [it, was_emplaced] = node2sceneid.emplace(node, primary_scene_id);
    assert(was_emplaced);

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

    auto populate_array_entry = [&](
        jsoncons::json&  j,
        const Transform& tf,
        StrView          name,
        const size_t*    parent_id)
    {
        j["transform"] = transform_as_json(tf);
        if (name.size()) {
            j["name"] = name;
        }
        if (parent_id) {
            j["parent"] = *parent_id;
        }
    };

    // Populate the primary scene node.
    populate_array_entry(
        array[primary_scene_id],
        m2tf(node->mTransformation),
        s2sv(node->mName),
        // `node2sceneid` is populated in pre-order, so we should always find our parent there.
        try_find_value(node2sceneid, node->mParent)
    );

    if (node->mNumMeshes) {
        if (node->mNumMeshes == 1) {

            // If a node contains only a single mesh then it is directly associated with it.
            const auto mesh_id = node->mMeshes[0];
            meshid2sceneids.emplace(mesh_id, primary_scene_id);

        } else /* num_meshes > 1 */ {

            // If there are more than one mesh per node, then we create additional
            // child leaf nodes in our representation of the scene to accomodate that.
            for (const auto [i, mesh_id] : enumerate(make_span(node->mMeshes, node->mNumMeshes))) {
                const size_t leaf_scene_id = array.size();
                array.emplace_back();
                meshid2sceneids.emplace(mesh_id, leaf_scene_id);

                populate_array_entry(
                    array[leaf_scene_id],
                    Transform(),                             // Identity transform.
                    s2sv(ai_scene->mMeshes[mesh_id]->mName), // Get name from the mesh.
                    &primary_scene_id                        // Always has a parent node.
                );
            }

        }
    }


    for (const aiNode* child : make_span(node->mChildren, node->mNumChildren)) {
        populate_scene_nodes_preorder(array, ai_scene, child, node2sceneid, meshid2sceneids, node2bone);
    }
}


auto image_intent_colorspace(ImageIntent intent)
    -> TextureFile::Colorspace
{
    switch (intent) {
        using enum TextureFile::Colorspace;
        case ImageIntent::Albedo:
            return sRGB;
        case ImageIntent::Specular:
        case ImageIntent::Normal:
        case ImageIntent::Alpha:
        case ImageIntent::Heightmap:
        case ImageIntent::Unknown:
            return Linear;
        default:
            assert(false);
            return Linear;
    }
}


} // namespace


auto import_scene_async(
    AssetImporterContext context,
    Path                 path,
    ImportSceneParams    params)
        -> Job<UUID>
{
    co_await reschedule_to(context.thread_pool());

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

    const auto ai_meshes    = make_span(ai_scene->mMeshes,     ai_scene->mNumMeshes   ); // Order: Meshes.
    const auto ai_materials = make_span(ai_scene->mMaterials,  ai_scene->mNumMaterials); // Order: Materials.
    const auto ai_anims     = make_span(ai_scene->mAnimations, ai_scene->mNumAnimations);


    const auto get_job_result = [](auto&& job) { return job.get_result(); };


    // Texture loads are independent of anything else, they also are
    // the only resource that actually has to load extra data from disk.
    // So we launch texture jobs as early as possible, anticipating
    // that loading them will take the longest anyway.

    HashMap<const aiMaterial*, MaterialIDs> material2matids;
    HashMap<Path, TextureInfo>              path2texinfo;

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

    Vector<Job<UUID>>       texture_jobs;
    Vector<TextureJobIndex> texid2jobid;
    texid2jobid .resize (num_textures);
    texture_jobs.reserve(num_textures);

    for (const auto& item : path2texinfo) {
        const auto& [path, tex_info] = item;
        texid2jobid[tex_info.id] = texture_jobs.size();

        const ImportTextureParams tex_params{
            .encoding       = params.texture_encoding,
            .colorspace     = image_intent_colorspace(tex_info.intent),
            .generate_mips  = params.generate_mips,
        };

        texture_jobs.emplace_back(context.importer().import_asset(path, tex_params));
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

    HashMap<const aiNode*, const aiBone*>              node2bone;
    HashMap<const aiMesh*, const aiNode*>              mesh2armature;
    HashMap<const aiNode*, Vector<const aiAnimation*>> armature2anims;
    HashMap<const aiAnimation*, const aiNode*>         anim2armature;
    HashSet<const aiNode*>                             armatures; // Order: Skeleton.

    for (const aiMesh* ai_mesh : ai_meshes) {
        if (ai_mesh->HasBones()) {
            const auto bones = make_span(ai_mesh->mBones, ai_mesh->mNumBones);

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
                const auto channels = make_span(ai_anim->mChannels, ai_anim->mNumChannels);
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
    Vector<Job<UUID>> skeleton_jobs; // Order: Skeletons.

    using Node2JointID = HashMap<const aiNode*, size_t>;
    // Maps: Bone Node -> Joint ID per armature. The name is ridiculous.
    // Populated inside populate_joints_preorder() as the order is established.
    HashMap<const aiNode*, Node2JointID> armature2_node2jointid;

    skeleton_jobs         .reserve(armatures.size());
    armature2_node2jointid.reserve(armatures.size());

    // Submit skeleton jobs. This will also populate the respective entries in node2jointids.
    for (const aiNode* armature : armatures) {
        // Emplace empty. Populate when importing.
        auto [it, was_emplaced] = armature2_node2jointid.try_emplace(armature);
        assert(was_emplaced);
        Node2JointID& node2jointid = it->second;
        skeleton_jobs.emplace_back(import_skeleton_async(context.child_context(), armature, node2jointid, node2bone));
    }

    co_await until_all_ready(skeleton_jobs);
    co_await reschedule_to(context.thread_pool());


    // Now unpack the relationship betweed each armature and associated UUID.
    Vector<UUID> skeleton_uuids = // Order: Skeletons.
        skeleton_jobs | transform(get_job_result) | ranges::to<Vector>();

    skeleton_jobs.clear();

    HashMap<const aiNode*, UUID> armature2uuid =
        zip(armatures, skeleton_uuids) | ranges::to<HashMap<const aiNode*, UUID>>();



    // Finally, we can submit importing of Meshes and Animations,
    // so that they can reference correct Skeletons.
    //
    // NOTE: This is technically not required to be ordered like
    // this if we permit "patching" referenced skeletons in the
    // mesh and animation files after creating the files.
    // That would probably be better from task scheduling perspective
    // and performance, but the current way is just simpler.

    Vector<Job<UUID>> mesh_jobs; // Order: Meshes.
    Vector<Job<UUID>> anim_jobs; // Order: Anims.

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

        mesh_jobs.emplace_back(import_mesh_async(context.child_context(), ai_mesh, skeleton_uuid, node2jointid));
    }

    for (const aiAnimation* ai_anim : ai_anims) {
        const aiNode* armature           = anim2armature.at(ai_anim);
        const UUID skeleton_uuid         = armature2uuid.at(armature);
        const Node2JointID& node2jointid = armature2_node2jointid.at(armature);
        anim_jobs.emplace_back(import_anim_async(context.child_context(), ai_anim, armature, skeleton_uuid, node2jointid));
    }


    // Wait for completion of texture jobs, so that we could assemble the Material files.


    co_await until_all_ready(texture_jobs);
    co_await reschedule_to(context.thread_pool());


    Vector<UUID> texture_uuids = texture_jobs | transform(get_job_result) | ranges::to<Vector>();


    // Material files just bundle together multiple textures plus
    // some other surface display parameters. We do these pretty late
    // because materials depend on textures and those usually take
    // the longest time to import.

    Vector<Job<UUID>> material_jobs; // Order: Materials
    material_jobs.reserve(ai_materials.size());


    for (const aiMaterial* ai_material : ai_materials) {

        const auto get_uuid_from_texid = [&](TextureIndex id) -> UUID {
            return id >= 0 ? texture_uuids[texid2jobid.at(id)] : UUID{};
        };

        const MaterialIDs mat = material2matids.at(ai_material);

        const MaterialUUIDs texture_uuids{
            .diffuse_uuid  = get_uuid_from_texid(mat.diffuse_id),
            .specular_uuid = get_uuid_from_texid(mat.specular_id),
            .normal_uuid   = get_uuid_from_texid(mat.normal_id),
        };

        const float specpower = 128.f; // Still using a dummy value. Ohwell.

        // Have to pass the name by-value since in this case it is a copy
        // and not a reference to a member field. Do not fret, my friend,
        // I'll just copy this 1KB string real quick and it'll be over.
        material_jobs.emplace_back(import_material_async(context.child_context(), String(ai_material->GetName().C_Str()), texture_uuids, specpower));
    }


    co_await until_all_ready(mesh_jobs);
    co_await until_all_ready(material_jobs);
    co_await reschedule_to(context.thread_pool());

    Vector<UUID> mesh_uuids     = mesh_jobs     | transform(get_job_result) | ranges::to<Vector>(); // Order: Meshes.
    Vector<UUID> material_uuids = material_jobs | transform(get_job_result) | ranges::to<Vector>(); // Order: Materials.

    // Mesh Description is a file that just references a Mesh+Material.
    // Sometimes this is referred to as a "Mesh Entity".

    Vector<Job<UUID>> mdesc_jobs; // Order: Meshes.

    mdesc_jobs.reserve(mesh_uuids.size());

    for (auto [ai_mesh, mesh_uuid] : zip(ai_meshes, mesh_uuids)) {
        const UUID material_uuid = material_uuids.at(ai_mesh->mMaterialIndex);

        // NOTE: Can pass aiString as string view here because consistency and assimp...
        mdesc_jobs.emplace_back(import_mesh_entity_async(context.child_context(), mesh_uuid, material_uuid, s2sv(ai_mesh->mName)));
    }


    co_await until_all_ready(anim_jobs);
    co_await until_all_ready(mdesc_jobs);
    co_await reschedule_to(context.thread_pool());


    Vector<UUID> mdesc_uuids = mdesc_jobs | transform(get_job_result) | ranges::to<Vector>(); // Order: Meshes.

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

    // Each assimp node can contain *multiple* meshes and we cannot represent that,
    // so we instead make the "multimesh" node a parent of N leaf nodes with identity
    // transformation and attach meshes to those leaves.
    // If a node only contains one mesh, no additional leaves are created.
    std::unordered_multimap<size_t, size_t> meshid2sceneids;

    // Some scene nodes, like multimesh leaves are not going to have
    // an association with a particular aiNode. This is only helpful
    // for lookup of camera and light nodes, not meshes.
    HashMap<const aiNode*, size_t> node2sceneid;

    populate_scene_nodes_preorder(
        entities_array,
        ai_scene,
        ai_scene->mRootNode,
        node2sceneid,
        meshid2sceneids,
        node2bone
    );

    // Handle each entity type: Meshes, Lights, Cameras.
    //
    // NOTE: I really hope that the same aiNode cannot be referenced by multiple entities at once.
    // That is, if, for example, a single node referenced both a Mesh and a Camera.
    // That would be completely unhinged and break many assumptions we have.
    // Assimp, please, be sane for once.

    // NOTE: Assimp was not sane for once.

    // NOTE: Meshes are found by references in the graph, since more than one
    // node can reference the same mesh (instancing).
    // Meshes *cannot* be found by name and their names are not even required to exist.
    for (const auto [i, mdesc_uuid] : enumerate(mdesc_uuids)) {
        const auto [beg, end] = meshid2sceneids.equal_range(i);
        for (const size_t scene_id : std::ranges::subrange(beg, end) | std::views::values) {
            // Lookup the array entry in the scene array and add the mesh component info.
            auto& e = entities_array[scene_id];
            e["type"] = "Mesh";
            e["uuid"] = serialize_uuid(mdesc_uuid);
        }
    }

    // NOTE: Lights are found by name lookup.
    // TODO: Care should be taken to handle cases where a node is both a Light *and* a Mesh.
    // We could separate those nodes as leafs similar to how we split meshes.
    for (const aiLight* ai_light : make_span(ai_scene->mLights, ai_scene->mNumLights)) {
        const aiNode* node = ai_scene->mRootNode->FindNode(ai_light->mName);
        const size_t scene_id = node2sceneid.at(node);
        auto& e = entities_array[scene_id];
        // TODO: Actually fill out the fields?

    }

    // NOTE: Cameras are found by name lookup.
    for (const aiCamera* ai_camera : make_span(ai_scene->mCameras, ai_scene->mNumCameras)) {
        // TODO:
        // ai_camera->mName;
    }

    // RANT: Assimp cannot decide whether it wants to do lookup "by-name", "by-pointer" or "by-index".
    // Guuys, can you just pick one and stick with it? No? Okaaay ;_;


    json scene_json;
    scene_json["entities"] = MOVE(entities_array);

    const ResourcePathHint path_hint{
        .directory = "scenes",
        .name      = s2sv(ai_scene->mName),
        .extension = "jscene",
    };

    const ResourceType resource_type = RT::Scene;

    scene_json["resource_type"] = resource_type;
    scene_json["self_uuid"]     = serialize_uuid(UUID{}); // Write null uuid to create space.

    String scene_json_string;
    scene_json.dump(scene_json_string, jsoncons::indenting::indent);
    const size_t file_size = scene_json_string.size();


    auto [uuid, mregion] = context.resource_database().generate_resource(resource_type, path_hint, file_size);


    // FIXME: This is very dumb, but I have no idea of a better way of doing this.
    // We re-dump the json into the string, just to replace the nil UUID with the real one.
    scene_json["self_uuid"] = serialize_uuid(uuid);
    scene_json_string.clear();
    scene_json.dump(scene_json_string, jsoncons::indenting::indent);
    assert(file_size == scene_json_string.size());


    // Write the scene info to the file.
    {
        const auto dst_bytes = to_span(mregion);
        const auto src_bytes = as_bytes(to_span(scene_json_string));
        assert(src_bytes.size() == dst_bytes.size());
        std::ranges::copy(src_bytes, dst_bytes.begin());
    }

    // TODO: Each job can fail. How to handle?
    // Use remove_resource_later()?

    co_return uuid;
}


} // namespace josh::detail
