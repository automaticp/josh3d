#include "Throughporters.hpp"
#include "AnimationStorage.hpp"
#include "AsyncCradle.hpp"
#include "Camera.hpp"
#include "CommonConcepts.hpp"
#include "CompletionContext.hpp"
#include "Components.hpp"
#include "ContainerUtils.hpp"
#include "CoroCore.hpp"
#include "Coroutines.hpp"
#include "ECS.hpp"
#include "Elements.hpp"
#include "EnumUtils.hpp"
#include "ExternalScene.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include "GLTextures.hpp"
#include "ImageData.hpp"
#include "ImageProperties.hpp"
#include "LightCasters.hpp"
#include "Materials.hpp"
#include "MeshRegistry.hpp"
#include "MeshStorage.hpp"
#include "Name.hpp"
#include "Processing.hpp"
#include "RuntimeError.hpp"
#include "SceneGraph.hpp"
#include "SkeletalAnimation.hpp"
#include "SkeletonStorage.hpp"
#include "SkinnedMesh.hpp"
#include "StaticMesh.hpp"
#include "Transform.hpp"
#include "VertexFormat.hpp"
#include "VertexSkinned.hpp"
#include "detail/CGLTF.hpp"
#include "AlphaTested.hpp"
#include <glbinding/gl/functions.h>
#include <stb_image.h>
#include <ranges>


namespace josh {


JOSH3D_DERIVE_TYPE(BaseTextureJob, Job<UniqueTexture2D>);

/*
NOTE: Will update width, height and num_channels of the image.
*/
auto load_or_decode_then_upload_esr_image(
    esr::Image&       image,
    const Path&       base_dir,
    AsyncCradleRef    async)
        -> BaseTextureJob
{
    co_await reschedule_to(async.loading_pool);
    ImageData<ubyte> imdata = load_or_decode_esr_image(image, base_dir);
    image.width        = imdata.resolution().width;
    image.height       = imdata.resolution().height;
    image.num_channels = imdata.num_channels();

    co_await reschedule_to(async.offscreen_context);
    UniqueTexture2D texture = upload_base_image_data(imdata);

    co_await async.completion_context.until_ready_on(async.offscreen_context, create_fence());

    co_return texture;
}

// TODO: Should be part of gl.
[[nodiscard]]
auto create_texture_view(
    RawTexture2D<GLConst> src,
    InternalFormat        iformat)
        -> UniqueTexture2D
{
    GLuint id;
    gl::glGenTextures(1, &id);
    gl::glTextureView(
        id,
        enum_cast<gl::GLenum>(src.target_type),
        src.id(),
        enum_cast<gl::GLenum>(iformat),
        0,                         // min level
        src.get_num_view_levels(), // num levels
        0,                         // min layer
        1                          // num layers
    );
    return UniqueTexture2D::take_ownership(RawTexture2D<>::from_id(id));
}

JOSH3D_DERIVE_TYPE(TextureViewJob, Job<SharedTexture2D>);

auto await_base_image_then_create_texture_view(
    const esr::ExternalScene& scene,
    const esr::Texture&       texture,
    AsyncCradleRef            async)
        -> TextureViewJob
{
    const esr::ImageID image_id = texture.image_id;
    const esr::Image&  image    = scene.get<esr::Image>(image_id);

    // NOTE: We cannot directly co_await the image jobs because multiple
    // textures could try to await a single image job, and that does not work
    // that way (can't have multiple continuations, will assert). We can instead
    // just poll. There might be a better way than polling but this is sufficent.
    const auto& job = scene.get<BaseTextureJob>(image_id);
    co_await async.completion_context.until_ready(job);
    co_await reschedule_to(async.offscreen_context);

    const InternalFormat iformat = ubyte_color_iformat(image.num_channels, texture.colorspace);
    const RawTexture2D<GLConst> base_texture = job.get_result();

    UniqueTexture2D view = create_texture_view(base_texture, iformat);

    view->set_swizzle_rgba(texture.swizzle);
    view->set_sampler_wrap_s(texture.sampler_info.wrap_s);
    view->set_sampler_wrap_t(texture.sampler_info.wrap_t);
    view->set_sampler_min_filter(texture.sampler_info.min_filter);
    view->set_sampler_mag_filter(texture.sampler_info.mag_filter);

    co_return view;
}

// NOTE: Infer the concrete type of the vertex from the esr::Mesh itself.
JOSH3D_DERIVE_TYPE(MeshJob, Job<MeshID<>>);

auto pack_and_upload_mesh_data(
    const esr::Mesh& mesh,
    MeshRegistry&    mesh_registry,
    AsyncCradleRef   async)
        -> MeshJob
{
    co_await reschedule_to(async.loading_pool);

    if (mesh.format == VertexFormat::Skinned)
    {
        validate_attributes_skinned(mesh.attributes);

        Vector<u32> indices =
            pack_indices(mesh.attributes.indices);

        Vector<VertexSkinned> verts =
            pack_attributes_skinned(
                mesh.attributes.positions,
                mesh.attributes.uvs,
                mesh.attributes.normals,
                mesh.attributes.tangents,
                mesh.attributes.joint_ids,
                mesh.attributes.joint_ws);

        const MeshID<VertexSkinned> mesh_id =
            co_await upload_skinned_mesh(verts, indices, mesh_registry, async);

        co_return mesh_id;
    }

    if (mesh.format == VertexFormat::Static)
    {
        validate_attributes_static(mesh.attributes);

        Vector<u32> indices =
            pack_indices(mesh.attributes.indices);

        Vector<VertexStatic> verts =
            pack_attributes_static(
                mesh.attributes.positions,
                mesh.attributes.uvs,
                mesh.attributes.normals,
                mesh.attributes.tangents);

        const MeshID<VertexStatic> mesh_id =
            co_await upload_static_mesh(verts, indices, mesh_registry, async);

        co_return mesh_id;
    }

    panic_fmt("Invalid VertexFormat: {}.", to_underlying(mesh.format));
}

JOSH3D_DERIVE_TYPE(SkeletonJob, Job<SkeletonID>);

auto pack_and_insert_skeleton(
    const esr::Skin& skin,
    SkeletonStorage& storage,
    AsyncCradleRef   async)
        -> SkeletonJob
{
    co_await reschedule_to(async.loading_pool);

    auto j2j = [](const esr::Joint& j) -> Joint { return { j.inv_bind, j.parent_idx }; };

    Skeleton skeleton = {
        .joints = skin.joints | transform(j2j) | ranges::to<Vector>(),
        .name   = skin.name,
    };

    co_await reschedule_to(async.local_context);

    const SkeletonID id = storage.insert(MOVE(skeleton));

    co_return id;
}

JOSH3D_DERIVE_TYPE(AnimationJob, Job<AnimationID>);

auto await_skeleton_then_pack_and_insert_animation(
    const esr::SkinAnimation& animation,
    const esr::ExternalScene& scene,
    AnimationStorage&         storage,
    AsyncCradleRef            async)
        -> AnimationJob
{
    co_await reschedule_to(async.loading_pool);

    // TODO: This bit here is a separate function maybe?
    // TODO: We could also support a flavor that resamples at a fixed rate instead.

    const auto tps      = animation.tps == 0.0f ? animation.tps : 30.f;
    const auto duration = animation.duration * tps;

    Animation2lip clip = {
        .duration    = duration,
        .keyframes   = {}, // Will fill next.
        .name        = animation.name,
        .skeleton_id = nullid, // Will fill after the skeleton is ready.
    };

    const auto& skin       = scene.get<esr::Skin>(animation.skin_id);
    const usize num_joints = skin.joints.size();

    // NOTE: We resize to fill all channels with empty keyframes.
    // Then we overwrite only affected joints.
    clip.keyframes.resize(num_joints);

    for (const auto& [joint_idx, motion] : animation.motions)
    {
        // NOTE: Ignoring interpolation here for now.
        auto& joint_motion = clip.keyframes[joint_idx];

        auto emplace_channel = [&]<typename T>(
            const esr::MotionChannel&      src_channel,
            Vector<Animation2lip::Key<T>>& dst_channel)
        {
            const usize num_samples = src_channel.ticks.element_count;
            if (not num_samples) return;

            // NOTE: Will avoid batch conversion functions to not screw
            // up the conversion correctness. Plus the destination times
            // are in `double` and need to be scaled anyway.

            // FIXME: Oh god, what if the quaternions are not xyzw?

            dst_channel.reserve(num_samples);
            for (const uindex i : irange(num_samples))
            {
                const auto t = copy_convert_one_element<float>(src_channel.ticks,  i);
                const auto v = copy_convert_one_element<vec3> (src_channel.values, i);

                dst_channel.push_back({
                    .time  = t * tps,
                    .value = v,
                });
            }
        };

        emplace_channel(motion.translation, joint_motion.t);
        emplace_channel(motion.rotation,    joint_motion.r);
        emplace_channel(motion.scaling,     joint_motion.s);
    }

    // TODO: Maaaybe we would like to wait on a single job more efficiently?
    const auto& job = scene.get<SkeletonJob>(animation.skin_id);
    co_await async.completion_context.until_ready(job);

    clip.skeleton_id = job.get_result();

    co_await reschedule_to(async.local_context);

    const AnimationID id = storage.insert(MOVE(clip));

    co_return id;
}

/*
Identifies esr::Mesh or esr::Material that hasn't yet been
fully unpacked into the scene registry.
*/
struct PendingUnpacking
{
    esr::vector<Entity> targets;
};

auto tag_and_assemble_scene_graph(
    esr::NodeID         node_id,
    esr::ExternalScene& scene,
    Registry&           registry)
        -> Entity
{
    assert(node_id != esr::null_id);

    const auto& node = scene.get<esr::Node>(node_id);

    const Entity target        = registry.create();
    const Handle target_handle = { registry, target };
    registry.emplace<Transform>(target, node.transform);
    registry.emplace<Name>     (target, node.name);

    // Here we only mark the entities with PendingUnpacking,
    // the actual unpacking will be done in a separate pass.
    for (const esr::EntityID entity_id : node.entities)
    {
        auto& pending = scene.get_or_emplace<PendingUnpacking>(entity_id);
        pending.targets.push_back(target);
    }

    // Then iterate children.
    esr::NodeID child_id = node.child0_id;
    while (child_id != esr::null_id)
    {
        const Entity new_child = tag_and_assemble_scene_graph(child_id, scene, registry);
        attach_child(target_handle, new_child);
        child_id = scene.get<esr::Node>(child_id).sibling_id;
    }

    return target;
}

/*
Cameras, Lights, maybe other stuff without dependencies.
*/
auto unpack_pending_other(
    esr::EntityID       entity_id,
    esr::ExternalScene& scene,
    Handle              dst_handle,
    AsyncCradleRef      async)
        -> Job<>
{
    // TODO: We currently have no data inside esr::Camera or esr::Light
    // so there isn't much we can emplace here. I'll just stuff the
    // defaults in there.
    co_await reschedule_to(async.local_context);

    if (not dst_handle.valid())
        co_return;

    if (const auto* camera = scene.try_get<esr::Camera>(entity_id))
    {
        dst_handle.emplace<Camera>(Camera::Params{});
    }

    if (const auto* light = scene.try_get<esr::Light>(entity_id))
    {
        dst_handle.emplace<PointLight>();
    }
}

auto await_resource_and_unpack_pending_mesh(
    esr::MeshID         mesh_id,
    esr::ExternalScene& scene,
    Handle              dst_handle,
    AsyncCradleRef      async)
        -> Job<>
{
    assert(scene.any_of<esr::Mesh>(mesh_id));

    const auto& mesh     = scene.get<esr::Mesh>(mesh_id);
    const auto& mesh_job = scene.get<MeshJob>(mesh_id);

    // Prep storage for material's texture jobs.
    SmallVector<ReadyableRef<const TextureViewJob>, 4> texture_jobs;

    const esr::Material* material = scene.try_get<esr::Material>(mesh.material_id);
    if (material)
    {
        auto add_slot = [&](esr::TextureID texture_id)
        {
            if (texture_id != esr::null_id)
            {
                const auto& job = scene.get<TextureViewJob>(texture_id);
                texture_jobs.push_back(ReadyableRef(&job));
            }
        };

        add_slot(material->color_id);
        add_slot(material->normal_id);
        add_slot(material->specular_id);
        add_slot(material->specular_color_id);
    }

    co_await async.completion_context.until_ready(mesh_job);

    const SkeletonJob* skeleton_job = nullptr;
    if (mesh.skin_id != esr::null_id)
    {
        skeleton_job = &scene.get<SkeletonJob>(mesh.skin_id);
        co_await async.completion_context.until_ready(*skeleton_job);
    }

    // We can do first emplacement once the mesh(+skin) is ready.
    co_await reschedule_to(async.local_context);

    if (not dst_handle.valid())
        co_return;

    if (mesh.format == VertexFormat::Static)
    {
        LODPack<MeshID<VertexStatic>, 8> lods = {};
        lods[0] = mesh_job.get_result().as<VertexStatic>();
        insert_component<StaticMesh>(dst_handle, {
            .lods  = lods,
            .usage = ResourceUsage(), // No usage.
        });
    }
    else if (mesh.format == VertexFormat::Skinned)
    {
        LODPack<MeshID<VertexSkinned>, 8> lods = {};
        lods[0] = mesh_job.get_result().as<VertexSkinned>();
        // FIXME: "-1" hack should be formalized.
        const SkeletonID skeleton_id = skeleton_job ? skeleton_job->get_result() : SkeletonID(-1);
        insert_component<SkinnedMe3h>(dst_handle, {
            .lods           = lods,
            .usage          = ResourceUsage(), // No usage.
            .skeleton_id    = skeleton_id,
            .skeleton_usage = ResourceUsage(),
        });
    }

    insert_component(dst_handle, mesh.aabb);

    // TODO: The materials are a pain. This should be done with when_any(),
    // but we don't have that yet in either coroutines or the completion context.
    if (material)
    {
        co_await async.completion_context.until_all_ready(texture_jobs);
        co_await reschedule_to(async.local_context);

        if (not dst_handle.valid())
            co_return;

        if (material->color_id != esr::null_id)
        {
            const auto& job = scene.get<TextureViewJob>(material->color_id);

            glapi::make_available<Binding::Texture2D>(job.get_result()->id());

            insert_component<MaterialDiffuse>(dst_handle, {
                .texture = job.get_result(),
                .usage   = ResourceUsage(),
            });

            if (material->alpha_method == esr::AlphaMethod::Test)
                set_tag<AlphaTested>(dst_handle);

            // NOTE: Ignoring double_sided for now. It is always double sided if AlphaTested.
            // NOTE: Ignoring all of the "factors", material setup does not support them.
        }

        if (material->normal_id != esr::null_id)
        {
            const auto& job = scene.get<TextureViewJob>(material->normal_id);

            glapi::make_available<Binding::Texture2D>(job.get_result()->id());

            insert_component<MaterialNormal>(dst_handle, {
                .texture = job.get_result(),
                .usage   = ResourceUsage(),
            });
        }

        if (material->specular_id != esr::null_id)
        {
            const auto& job = scene.get<TextureViewJob>(material->specular_id);

            // FIXME: This is mutating an existing texture view.
            // The shaders currently expect the Red channel to have
            // data. This isn't even the right "specular" anyway...
            auto& texture = job.get_result();

            glapi::make_available<Binding::Texture2D>(job.get_result()->id());

            using enum Swizzle;
            const SwizzleRGBA swizzle      = { Alpha, Zero, Zero, Zero };
            const SwizzleRGBA full_swizzle = fold_swizzle(texture->get_swizzle_rgba(), swizzle);
            texture->set_swizzle_rgba(full_swizzle);

            insert_component<MaterialSpecular>(dst_handle, {
                .texture   = job.get_result(),
                .usage     = ResourceUsage(),
                .shininess = 128.f, // Hahaha, still no idea where to get this.
            });
        }
    }
}

JOSH3D_DERIVE_TYPE(SceneJob, Job<>);

auto assemble_and_unpack_scene(
    const esr::Scene&         scene_,
    esr::ExternalScene&       scene,
    Entity                    dst_entity,
    Registry&                 registry,
    AsyncCradleRef            async)
        -> SceneJob
{
    co_await reschedule_to(async.local_context);

    // Assemble the scene graph first.
    // Then repeatedly wait until Meshes and Materials are ready.

    // HMM: I think scenes can "instance" nodes. This is crazy.
    // It is easier to just create entities one-by-one.

    for (const esr::NodeID root_id : scene_.root_node_ids)
    {
        const Entity root_ent =
            tag_and_assemble_scene_graph(root_id, scene, registry);

        if (dst_entity != nullent and registry.valid(dst_entity))
            attach_to_parent({ registry, root_ent }, dst_entity);
    }

    co_await reschedule_to(async.loading_pool);

    // The above has emplaced PendingUnpacking where necessary.
    // We go back to the thread pool to not block the main thread
    // and then we submit unpacking jobs for every *source* esr::Entity.

    Vector<Job<>> unpack_jobs; unpack_jobs.reserve(scene.view<PendingUnpacking>().size());
    for (const auto [entity_id, pending] : scene.view<PendingUnpacking>().each())
    {
        // FIXME: We are doing this in this dumb way where we submit a job per-destination
        // even if the job per-source would suffice. I'm just lazy right now to write
        // that properly. This also means that the number of actual jobs can exceed
        // the capacity reserved.
        for (const Entity target : pending.targets)
        {
            const Handle dst_handle = { registry, target };

            if (scene.any_of<esr::Mesh>(entity_id))
                unpack_jobs.push_back(await_resource_and_unpack_pending_mesh(entity_id, scene, dst_handle, async));

            if (scene.any_of<esr::Camera, esr::Light>(entity_id))
                unpack_jobs.push_back(unpack_pending_other(entity_id, scene, dst_handle, async));
        }
    }

    co_await until_all_succeed(unpack_jobs);
}

auto throughport_external_scene(
    esr::ExternalScene   scene,
    Entity               dst_entity,
    ESRThroughportParams params,
    ThroughportContext   context)
        -> Job<>
{
    auto& async = context.async_cradle;

    co_await reschedule_to(async.loading_pool);

    // Time to get our hands dirty.
    //
    // What we can do is emplace loading/unpacking jobs directly into
    // the respective data entities. This will save us from the headache
    // of keeping arrays here and trying to reconstruct the references
    // between entities from *order* of those arrays.

    // HMM: When it comes to animations, we likely want to get *an* ID
    // ASAP, before any data is loaded or the skeletons are finalized,
    // so that the animation system could just have an animation in
    // a playing state (without it being applied yet). Do we?


    // Launch all image loading / decoding / uploading jobs.

    for (auto [image_id, image] : scene.view<esr::Image>().each())
        scene.emplace<BaseTextureJob>(image_id,
            load_or_decode_then_upload_esr_image(image, scene.base_dir, async));

    // Launch all texture uploads.
    // The "textures" here are the same as the images. And are attached to them.
    // We can simply upload all of the images, and then create views from them,
    // optionally swizzling them in the process.

    // These will wait on their referenced base texture jobs, then create views from them.
    for (auto [texture_id, texture] : scene.view<esr::Texture>().each())
        scene.emplace<TextureViewJob>(texture_id,
            await_base_image_then_create_texture_view(scene, texture, async));

    // Postpone loading of materials until we have the scene entities to associate them with.
    // For now, handle loading the meshes or something.
#if 0
    for (const auto [material_id, material] : scene.view<esr::Material>().each())
    {
        auto ensure_is_loading = [&](esr::TextureID texture_id)
        {
            if (texture_id == esr::null_id) return;

            // Make sure the image data is being loaded.
            const esr::ImageID image_id = scene.get<esr::Texture>(texture_id).image_id;
            if (not scene.any_of<BaseTextureJob>(image_id))
            {
                const esr::Image& image = scene.get<esr::Image>(image_id);
                scene.emplace<BaseTextureJob>(image_id,
                    load_or_decode_then_upload_esr_image(image, scene.base_dir, async));
            }

            // Then launch a job to wait on the image, and create a view from it.

            // scene.emplace<
        };

        ensure_is_loading(material.albedo_id);
        ensure_is_loading(material.alpha_id);
        ensure_is_loading(material.normal_id);
        ensure_is_loading(material.specular_id);
        ensure_is_loading(material.specular_color_id);
    }
#endif

    // Start loading the meshes in the meantime.
    for (auto [mesh_id, mesh] : scene.view<esr::Mesh>().each())
        scene.emplace<MeshJob>(mesh_id,
            pack_and_upload_mesh_data(mesh, context.mesh_registry, async));

    // Load skeletons and animations.
    for (auto [skin_id, skin] : scene.view<esr::Skin>().each())
        scene.emplace<SkeletonJob>(skin_id,
            pack_and_insert_skeleton(skin, context.skeleton_storage, async));

    // NOTE: We are ignoring `esr::Animation` entirely since we have no way
    // to represent a "mixed" animation like that. We'll just import the
    // skeletal animations alone.
    for (auto [anim_id, anim] : scene.view<esr::SkinAnimation>().each())
        scene.emplace<AnimationJob>(anim_id,
            await_skeleton_then_pack_and_insert_animation(anim, scene, context.animation_storage, async));

    // NOTE: We have to unitarize before we assemble the scene in the Registry,
    // since the scene Registry cannot have duplicate components per enitity.
    // We have some time, while all other data is being loaded.
    unitarize_external_scene(scene, params.unitarization);

    // Yeah, `ExternalScene` is a collection of Scenes.
    // We should call them "Subscene" or something.
    for (auto [scene_id, scene_] : scene.view<esr::Scene>().each())
        scene.emplace<SceneJob>(scene_id,
            assemble_and_unpack_scene(scene_, scene, dst_entity, context.registry, async));

    co_await until_all_succeed(scene.storage<SceneJob>());
}

auto throughport_scene_gltf(
    Path                  path,
    Entity                dst_entity,
    GLTFThroughportParams params,
    ThroughportContext    context)
        -> Job<>
{
    using namespace detail::cgltf;
    auto& async = context.async_cradle;

    co_await reschedule_to(async.loading_pool);

    const cgltf_options options = {};
    cgltf_data*         gltf    = {};
    cgltf_result        result  = {};

    result = cgltf_parse_file(&options, path.c_str(), &gltf);
    if (result != cgltf_result_success)
        throw_fmt<GLTFParseError>("Failed to parse gltf file {}, reason {}.", path, enum_string(result));

    const auto _owner = unique_data_ptr(gltf);

    result = cgltf_load_buffers(&options, gltf, path.c_str());
    if (result != cgltf_result_success)
        throw_fmt<GLTFParseError>("Failed to load gltf buffers of {}, reason {}.", path, enum_string(result));

    esr::ExternalScene scene = to_external_scene(*gltf, path.parent_path());

    const ESRThroughportParams esr_params = {
        .generate_mips = params.generate_mips,
        .unitarization = params.unitarization,
    };

    co_await throughport_external_scene(MOVE(scene), dst_entity, esr_params, context);
}

auto throughport_scene_assimp(
    Path                    path,
    Entity                  dst_entity,
    AssimpThroughportParams params,
    ThroughportContext      context)
        -> Job<>
{
    // TODO:
    panic("Assimp throughporter is not implemented.");
}



} // namespace josh
