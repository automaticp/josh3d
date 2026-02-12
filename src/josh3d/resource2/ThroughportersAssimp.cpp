#include "Throughporters.hpp"
#include "Asset.hpp"
#include "AsyncCradle.hpp"
#include "Common.hpp"
#include "ContainerUtils.hpp"
#include "CoroCore.hpp"
#include "GLAPIBinding.hpp"
#include "GLBuffers.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLPixelPackTraits.hpp"
#include "GLTextures.hpp"
#include "Logging.hpp"
#include "MeshRegistry.hpp"
#include "MeshStorage.hpp"
#include "Scalars.hpp"
#include "SkeletalAnimation.hpp"
#include "Skeleton.hpp"
#include "TextureHelpers.hpp"
#include "VertexFormats.hpp"
#include "detail/AssimpCommon.hpp"
#include "detail/AssimpSceneRepr.hpp"
#include <assimp/Importer.hpp>
#include <assimp/anim.h>
#include <assimp/importerdesc.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/BaseImporter.h>
#include <boost/container_hash/hash.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <iterator>


/*
TODO: This all needs to be converted to produce ExternalScene instead.
It is not that different from AssimpSceneRepr.
*/
namespace josh {
namespace {

using namespace detail;
using asr = AssimpSceneRepr;

auto image_intent_internal_format(ImageIntent intent, size_t num_channels)
    -> InternalFormat
{
    switch (intent)
    {
        case ImageIntent::Albedo:
            switch (num_channels)
            {
                case 3: return InternalFormat::SRGB8;
                case 4: return InternalFormat::SRGBA8;
            }
        default:
            switch (num_channels)
            {
                case 1: return InternalFormat::R8;
                case 2: return InternalFormat::RG8;
                case 3: return InternalFormat::RGB8;
                case 4: return InternalFormat::RGBA8;
            }
    }
    throw_fmt<AssetError>("No InternalFormat for ImageIntent {} and {} channels.", enum_string(intent), num_channels);
}

auto pixel_data_format(size_t num_channels)
    -> PixelDataFormat
{
    switch (num_channels)
    {
        case 1: return PixelDataFormat::Red;
        case 2: return PixelDataFormat::RG;
        case 3: return PixelDataFormat::RGB;
        case 4: return PixelDataFormat::RGBA;
    }
    throw_fmt<AssetError>("No PixelDataFormat for {} channels.", num_channels);
}

auto load_texture(
    Path           path,
    ImageIntent    intent,
    bool           generate_mips,
    AsyncCradleRef async_cradle)
        -> Job<UniqueTexture2D>
{
    co_await reschedule_to(async_cradle.loading_pool);

    const auto [min, max] = image_intent_minmax_channels(intent);
    ImageData<ubyte> image_data = load_image_data_from_file<ubyte>(File(path), min, max);

    co_await reschedule_to(async_cradle.offscreen_context);

    UniqueTexture2D texture;
    const Extent2I        resolution0 = Extent2I(image_data.resolution());
    const NumLevels       num_levels  = generate_mips ? max_num_levels(resolution0) : NumLevels(1);
    const InternalFormat  iformat     = image_intent_internal_format(intent, image_data.num_channels());
    const PixelDataFormat pdformat    = pixel_data_format(image_data.num_channels());
    const PixelDataType   pdtype      = PixelDataType::UByte;
    texture->allocate_storage(resolution0, iformat, num_levels);
    texture->upload_image_region({ {}, resolution0 }, pdformat, pdtype, image_data.data());
    if (generate_mips) texture->generate_mipmaps();
    texture->set_sampler_min_mag_filters(MinFilter::LinearMipmapLinear, MagFilter::Linear);

    co_await async_cradle.completion_context.until_ready_on(async_cradle.offscreen_context, create_fence());
    co_return texture;
}

enum class VertexType
{
    Static,
    Skinned,
};

struct Mesh
{
    VertexType vertex_type;
    MeshID<>   mesh_id;
};

template<typename VertexT>
[[nodiscard]]
auto upload_mesh(
    Span<VertexT>         verts_data,
    Span<u32>             elems_data,
    MeshStorage<VertexT>& mesh_storage,
    AsyncCradleRef        async)
        -> Job<MeshID<VertexT>>
{
    co_await reschedule_to(async.offscreen_context);

    const StoragePolicies policies = {
        .mode        = StorageMode::StaticServer,
        .mapping     = PermittedMapping::NoMapping,
        .persistence = PermittedPersistence::NotPersistent,
    };
    UniqueBuffer<VertexT> verts_staging = specify_buffer(to_span(verts_data), policies);
    UniqueBuffer<u32>     elems_staging = specify_buffer(to_span(elems_data), policies);

    co_await async.completion_context.until_ready_on(async.local_context, create_fence());

    glapi::make_available<Binding::ArrayBuffer>       (verts_staging->id());
    glapi::make_available<Binding::ElementArrayBuffer>(elems_staging->id());

    const MeshID<VertexT> mesh_id = mesh_storage.insert_buffer(verts_staging, elems_staging);

    co_return mesh_id;
}

[[nodiscard]]
auto load_static_mesh(
    const asr::Mesh&           mesh,
    MeshStorage<VertexStatic>& storage,
    AsyncCradleRef             async)
        -> Job<Mesh>
{
    co_await reschedule_to(async.loading_pool);

    Vector<VertexStatic> verts_data = pack_static_mesh_verts(mesh.ptr);
    Vector<u32>          elems_data = pack_mesh_elems(mesh.ptr);

    const MeshID<VertexStatic> mesh_id =
        co_await upload_mesh(to_span(verts_data), to_span(elems_data), storage, async);

    co_return Mesh{ VertexType::Static, mesh_id };
}

[[nodiscard]]
auto load_skinned_mesh(
    const asr::Mesh&            mesh,
    MeshStorage<VertexSkinned>& storage,
    AsyncCradleRef              async)
        -> Job<Mesh>
{
    co_await reschedule_to(async.loading_pool);

    Vector<VertexSkinned> verts_data = pack_skinned_mesh_verts(mesh.ptr, mesh.boneid2jointid);
    Vector<u32>           elems_data = pack_mesh_elems(mesh.ptr);

    const MeshID<VertexSkinned> mesh_id =
        co_await upload_mesh(to_span(verts_data), to_span(elems_data), storage, async);

    co_return Mesh{ VertexType::Skinned, mesh_id };
}

[[nodiscard]]
auto load_skeleton(
    const asr::Armature& armature,
    AsyncCradleRef       async)
        -> Job<Skeleton>
{
    co_await reschedule_to(async.loading_pool);

    auto j2j = [](const asr::Joint& j) -> Joint
    {
        return { .inv_bind=j.inv_bind, .parent_idx=j.parent_idx };
    };

    co_return Skeleton{
        .joints = armature.joints | transform(j2j) | ranges::to<Vector<Joint>>(),
    };
}

[[nodiscard]]
auto load_anim(
    const asr::Animation&  anim,
    const AssimpSceneRepr& scene,
    AsyncCradleRef         async)
        -> Job<AnimationClip>
{
    co_await reschedule_to(async.loading_pool);

    const asr::Armature& armature = scene.registry.get<asr::Armature>(anim.armature_id);
    const auto           ai_joint_motions = make_span(anim->mChannels, anim->mNumChannels);

    const double tps        = (anim->mTicksPerSecond != 0) ? anim->mTicksPerSecond : 30.0;
    const double duration_s = anim->mDuration / tps;

    Vector<AnimationClip::JointKeyframes> joint_keyframes(ai_joint_motions.size());

    for (const aiNodeAnim* ai_joint_motion : ai_joint_motions)
    {
        // Lookup by name again, bleugh.
        const asr::NodeID nodeid  = scene.name2nodeid.at(s2sv(ai_joint_motion->mNodeName));
        const u32         jointid = armature.nodeid2jointid.at(nodeid);

        auto& keyframes = joint_keyframes[jointid];

        // It is guaranteed by assimp that times are monotonically *increasing*.
        const auto ai_pos_keys = make_span(ai_joint_motion->mPositionKeys, ai_joint_motion->mNumPositionKeys);
        const auto ai_rot_keys = make_span(ai_joint_motion->mRotationKeys, ai_joint_motion->mNumRotationKeys);
        const auto ai_sca_keys = make_span(ai_joint_motion->mScalingKeys,  ai_joint_motion->mNumScalingKeys );

        const auto to_vec3_key = [&tps](const aiVectorKey& vk) -> AnimationClip::Key<vec3> { return { float(vk.mTime / tps), v2v(vk.mValue) }; };
        const auto to_quat_key = [&tps](const aiQuatKey&   qk) -> AnimationClip::Key<quat> { return { float(qk.mTime / tps), q2q(qk.mValue) }; };

        std::ranges::copy(ai_pos_keys | transform(to_vec3_key), std::back_inserter(keyframes.t));
        std::ranges::copy(ai_rot_keys | transform(to_quat_key), std::back_inserter(keyframes.r));
        std::ranges::copy(ai_sca_keys | transform(to_vec3_key), std::back_inserter(keyframes.s));
    }

    co_return AnimationClip{
        .duration  = duration_s,
        .keyframes = MOVE(joint_keyframes),
        .skeleton  = nullptr, // FIXME: Uhh, well we probably shouldn't reference things like this.
    };
}


} // namespace


auto throughport_scene(
    Path                   path,
    Handle                 dst_handle,
    AssimpThroughportParams params,
    AsyncCradleRef         async,
    MeshRegistry&          mesh_registry)
        -> Job<>
{
    co_await reschedule_to(async.loading_pool);

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

    // Yeah, this is dumb. I don't know a of better way.
    // Why is there no GetLastExtension() or similar?
    const bool is_obj = (path.extension() == ".obj");
    const bool height_as_normals = is_obj;

    if (!ai_scene) throw AssetFileImportFailure(ai_importer.GetErrorString(), { path });

    auto alloc = asr::allocator<>();
    auto scene = AssimpSceneRepr::from_scene(*ai_scene, alloc);
    auto& registry = scene.registry;

    logstream() << "Repr Contents:\n";
    for (const asr::Mesh& mesh : registry.storage<asr::Mesh>())
        logstream() << "Mesh: " << mesh->mName.C_Str() << "\n";
    for (const asr::Armature& armature : registry.storage<asr::Armature>())
        logstream() << "Armature: " << armature.name << "\n";
    for (const asr::Animation& anim : registry.storage<asr::Animation>())
        logstream() << "Anim: " << anim->mName.C_Str() << "\n";
    for (const asr::Texture& tex : registry.storage<asr::Texture>())
        logstream() << "Texture: " << tex.path << "\n";

    logstream() << "Scene Graph:\n";
    for (const asr::Node& node : registry.storage<asr::Node>())
    {
        for (const auto _ : irange(node.depth))
            logstream() << "  ";

        logstream() << node->mName.C_Str() << "\n";
    }

    // Textures.
    // Start loading textures first since they'd take the longest to complete.

    auto mattexture_jobs = Vector<Job<UniqueTexture2D>>();
    mattexture_jobs.reserve(registry.storage<asr::MatTexture>().size());

    for (const auto [mattexture_id, mattexture] : registry.view<asr::MatTexture>().each())
    {
        const auto& texture = registry.get<asr::Texture>(mattexture.texture_id);
        if (texture.embedded) throw AssetContentsParsingError("TODO: Embedded textures are not supported.");
        const ImageIntent intent = ai_texture_type_to_image_intent(mattexture.type, height_as_normals);
        mattexture_jobs.emplace_back(load_texture(parent_dir / texture.path, intent, params.generate_mips, async));
    }

    // Meshes.
    // No LODs are supported here.

    auto mesh_jobs = Vector<Job<Mesh>>();
    mesh_jobs.reserve(registry.storage<asr::Mesh>().size());

    auto load_mesh = [&](const asr::Mesh& mesh)
    {
        if (mesh->HasBones()) return load_skinned_mesh(mesh, *mesh_registry.storage_for<VertexSkinned>(), async);
        else                  return load_static_mesh(mesh, *mesh_registry.storage_for<VertexStatic>(), async);
    };

    for (const auto [mesh_id, mesh] : registry.view<asr::Mesh>().each())
        mesh_jobs.emplace_back(load_mesh(mesh));

    // Skeletons.
    // TODO: We still have no skeleton/animation pool.

    auto skeleton_jobs = Vector<Job<Skeleton>>();
    skeleton_jobs.reserve(registry.storage<asr::Armature>().size());

    // TODO: Uhh, instancing probably does not work... Try?

    for (const asr::Armature& armature : registry.storage<asr::Armature>())
        skeleton_jobs.emplace_back(load_skeleton(armature, async));

    // Animations.
    // TODO: Currently nowhere to store them. But we'll do them for completeness.

    auto anim_jobs = Vector<Job<AnimationClip>>();
    anim_jobs.reserve(registry.storage<asr::Animation>().size());

    for (const asr::Animation& anim : registry.storage<asr::Animation>())
        anim_jobs.emplace_back(load_anim(anim, scene, async));



    auto get_result     = transform([](auto&& job) { return job.get_result(); });
    auto extract_result = transform([](auto&& job) { return MOVE(job).get_result(); });


#if 0

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

    std::vector<Job<UniqueTexture2D>> texture_jobs;
    Vector<TextureJobIndex>           texid2jobid;
    texid2jobid .resize (num_textures);
    texture_jobs.reserve(num_textures);

    for (const auto& item : path2texinfo) {
        const auto& [path, tex_info] = item;
        texid2jobid[tex_info.id] = texture_jobs.size();

        texture_jobs.emplace_back(load_texture(path, tex_info.intent, params.generate_mips, async_cradle));
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
        if (const auto* armature = try_find_value(mesh2armature, ai_mesh)) {
            assert(ai_mesh->HasBones());
            const auto  skeleton_uuid = armature2uuid.at(*armature);
            const auto& node2jointid  = armature2_node2jointid.at(*armature);
            mesh_jobs.emplace_back(import_skinned_mesh_async(context.child_context(), ai_mesh, skeleton_uuid, node2jointid));
        } else /* static */ {
            mesh_jobs.emplace_back(import_static_mesh_async(context.child_context(), ai_mesh));
        }
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


    // Vector<UUID> texture_uuids = texture_jobs | transform(get_job_result) | ranges::to<Vector>();


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

    String scene_name = [&]() -> String {
        if (ai_scene->mName.length) {
            return fmt::format("{}-{}", path.stem(), s2sv(ai_scene->mName));
        } else {
            return path.stem();
        }
    }();

    const ResourcePathHint path_hint{
        .directory = "scenes",
        .name      = scene_name,
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
#endif

    co_return;
}


} // namespace josh
