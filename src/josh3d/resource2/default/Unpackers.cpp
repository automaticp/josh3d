#include "DefaultTextures.hpp"
#include "Resources.hpp"
#include "Common.hpp"
#include "Components.hpp"
#include "CoroCore.hpp"
#include "Coroutines.hpp"
#include "GLTextures.hpp"
#include "Materials.hpp"
#include "Ranges.hpp"
#include "ResourceRegistry.hpp"
#include "ResourceUnpacker.hpp"
#include "ECS.hpp"
#include "Scalars.hpp"
#include "SceneGraph.hpp"
#include "SkinnedMesh.hpp"
#include "StaticMesh.hpp"
#include "Tags.hpp"
#include "AlphaTested.hpp"


namespace josh {


auto unpack_static_mesh(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>
{
    /*
    On the first step we expect:
        - Handle is valid;
        - No relevant component is emplaced yet
          ("first-to-emplace" strategy);

    On repeated incremental steps we expect:
        - Handle is still valid;
        - The component is present;
        - The ABA tag is the same as ours;

    If the expectations are not met, we bail.

    FIXME: The ABA tag is intrusive to each component, it would be better
    to use a separate component that is "linked" to the primary one via
    some on_destroy<Component>() callback or similar.
    */
    const auto aba_tag = uintptr(co_await peek_coroutine_address());

    /*
    FIXME: When we "bail", we likely want to report this somehow,
    maybe throw, maybe log, but something needs to be done to
    notify that unpacking was interrupted.
    */
    const auto bail = [](){};

    ResourceEpoch epoch = null_epoch;

    // Initial step.
    {
        auto [resource, usage] =
            co_await context.resource_loader().get_resource<RT::StaticMesh>(uuid, &epoch);

        co_await reschedule_to(context.local_context());

        if (not handle.valid() or handle.any_of<LocalAABB, StaticMesh>())
            co_return bail();

        insert_component<StaticMesh>(handle, {
            .lods    = resource.lods,
            .usage   = MOVE(usage),
            .aba_tag = aba_tag
        });

        insert_component(handle, resource.aabb);
    }

    // Incremental updates.
    while (epoch != final_epoch)
    {
        auto [resource, usage] =
            co_await context.resource_loader().get_resource<RT::StaticMesh>(uuid, &epoch);

        co_await reschedule_to(context.local_context());

        if (not handle.valid() or not has_component<StaticMesh>(handle))
            co_return bail();

        auto& component = handle.get<StaticMesh>();

        if (component.aba_tag != aba_tag)
            co_return bail();

        // TODO: Should we update the usage too? Why would it change?
        component.lods = resource.lods;
    }
}

auto unpack_skinned_mesh(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>
{
    const auto aba_tag = uintptr(co_await peek_coroutine_address());
    const auto bail = [](){};

    ResourceEpoch epoch = null_epoch;

    {
        auto [resource, usage] =
            co_await context.resource_loader().get_resource<RT::SkinnedMesh>(uuid, &epoch);

        co_await reschedule_to(context.local_context());

        if (not handle.valid() or handle.any_of<LocalAABB, SkinnedMe2h>())
            co_return bail();

        // NOTE: Requesting a secondary Skeleton resource after the first LOD is loaded.
        // This is suboptimal. May consider updating first epoch with just the skeleton UUID.
        auto [skeleton_resource, skeleton_usage] =
            co_await context.resource_loader().get_resource<RT::Skeleton>(resource.skeleton_uuid);

        insert_component<SkinnedMe2h>(handle, {
            .lods           = resource.lods,
            .usage          = MOVE(usage),
            .skeleton       = skeleton_resource.skeleton,
            .skeleton_usage = skeleton_usage,
            .aba_tag        = aba_tag,
        });

        // NOTE: A bit dirty, but we need to emplace this to render skinned meshes.
        // Computing best be done outside of the main thread, but alas...
        insert_component(handle, Pose::from_skeleton(*skeleton_resource.skeleton));
        insert_component(handle, resource.aabb);
    }

    while (epoch != final_epoch)
    {
        auto [resource, usage] =
            co_await context.resource_loader().get_resource<RT::SkinnedMesh>(uuid, &epoch);

        co_await reschedule_to(context.local_context());

        if (not handle.valid() or not has_component<SkinnedMe2h>(handle))
            co_return bail();

        auto& component = handle.get<SkinnedMe2h>();

        if (component.aba_tag != aba_tag)
            co_return bail();

        component.lods = resource.lods;
    }
}

namespace {

auto unpack_material_texture(
    ResourceUnpackerContext&                 context,
    UUID                                     uuid,
    Handle                                   handle,
    uintptr                                  aba_tag,
    decltype(&MaterialPhong::diffuse)        slot_mptr,
    decltype(&MaterialPhong::diffuse_usage)  usage_slot_mptr,
    invocable<Handle, MaterialPhong&> auto&& post_init)
        -> Job<>
{
    const auto bail = [](){};

    ResourceEpoch epoch = null_epoch;
    {
        auto [resource, usage] =
            co_await context.resource_loader().get_resource<RT::Texture>(uuid, &epoch);

        co_await reschedule_to(context.local_context());

        if (not handle.valid())
            co_return bail();

        if (not has_component<MaterialPhong>(handle))
        {
            // NOTE: First to reach the handle initializes the whose component.
            // NOTE: I don't care anymore, I'll just emplace all defaults here.
            insert_component<MaterialPhong>(handle, {
                .diffuse   = globals::share_default_diffuse_texture(),
                .normal    = globals::share_default_normal_texture(),
                .specular  = globals::share_default_specular_texture(),
                .specpower = 128.f,
                .aba_tag   = aba_tag, // The only non-default.
            });
        }

        auto& mtl = handle.get<MaterialPhong>();

        if (mtl.aba_tag != aba_tag)
            co_return bail();

        mtl.*slot_mptr       = MOVE(resource.texture);
        mtl.*usage_slot_mptr = MOVE(usage);

        post_init(handle, mtl);
    }

    while (epoch != final_epoch)
    {
        auto [resource, usage] =
            co_await context.resource_loader().get_resource<RT::Texture>(uuid, &epoch);

        co_await reschedule_to(context.local_context());

        if (not handle.valid())
            co_return bail();

        if (not has_component<MaterialPhong>(handle))
            co_return bail();

        auto& mtl = handle.get<MaterialPhong>();

        if (mtl.aba_tag != aba_tag)
            co_return bail();

        mtl.*slot_mptr = MOVE(resource.texture);
    }
}

} // namespace

auto unpack_material(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>
{
    const auto aba_tag = uintptr(co_await peek_coroutine_address());
    auto [material, usage] = co_await context.resource_loader().get_resource<RT::Material>(uuid);

    // NOTE: Post-init functions. These are only invoked once per job.
    const auto set_atested = [](Handle handle, MaterialPhong& mtl)
    {
        if (mtl.diffuse->get_component_type<PixelComponent::Alpha>()
            != PixelComponentType::None)
        {
            set_tag<AlphaTested>(handle);
        }
    };

    const auto set_specpower = [pw=material.specpower](Handle, MaterialPhong& mtl)
    {
        mtl.specpower = pw;
    };

    const auto no_op = [](auto&&...) {};

    StaticVector<Job<>, 3> jobs;

    if (not material.diffuse_uuid.is_nil())
        jobs.push_back(unpack_material_texture(context, material.diffuse_uuid, handle, aba_tag,
            &MaterialPhong::diffuse, &MaterialPhong::diffuse_usage, set_atested));

    if (not material.normal_uuid.is_nil())
        jobs.push_back(unpack_material_texture(context, material.normal_uuid, handle, aba_tag,
            &MaterialPhong::normal, &MaterialPhong::normal_usage, no_op));

    if (not material.specular_uuid.is_nil())
        jobs.push_back(unpack_material_texture(context, material.specular_uuid, handle, aba_tag,
            &MaterialPhong::specular, &MaterialPhong::specular_usage, set_specpower));

    co_await until_all_succeed(jobs);
}

auto unpack_mdesc(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>
{
    auto [mdesc, usage] = co_await context.resource_loader().get_resource<RT::MeshDesc>(uuid);

    StaticVector<Job<>, 2> jobs;

    jobs.emplace_back(context.unpacker().unpack_any(mdesc.mesh_uuid, handle));
    jobs.emplace_back(context.unpacker().unpack<RT::Material>(mdesc.material_uuid, handle));

    co_await until_all_succeed(jobs);
}

auto unpack_scene(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>
{
    auto [scene, usage] = co_await context.resource_loader().get_resource<RT::Scene>(uuid);

    // We are going to start loading resources from the scene,
    // as well as emplacing them into the registry.
    auto  nodes    = to_span(*scene.nodes);
    auto& registry = *handle.registry();

    // NOTE: Not thread_local because we are jumping threads here.
    Vector<Entity> new_entities; new_entities.resize(nodes.size());
    Vector<Job<>>  entity_jobs;  entity_jobs.reserve(nodes.size());

    // TODO: The fact that the scene is loaded before any resources are is a bit
    // of an issue. Could we not do that somehow? Else there's at least 1 frame
    // lag between loading the scene, and its completion in the registry.
    //
    // Maybe have the entities array store some awaitable flag that each
    // per-object job can wait upon until the entity is actually emplaced
    // with from another job.
    //
    // Essentially, we want the "registry.create()" job to arrive first
    // to the queue, but not block until its done, and instead push more
    // per-object jobs to the queue right after, so that when the per-frame
    // "update" is called, we are likely to just resolve it all one-by-one.

    co_await reschedule_to(context.local_context());

    registry.create(new_entities.begin(), new_entities.end());

    for (const auto [node, entity] : zip(nodes, new_entities))
    {
        const Handle node_handle = { registry, entity };
        node_handle.emplace<Transform>(node.transform);
        if (node.parent_index != SceneResource::Node::no_parent)
        {
            attach_to_parent(node_handle, new_entities[node.parent_index]);
        }
        else /* root */
        {
            // NOTE: All root nodes are attached to the scene handle.
            // I might revise this or make it configurable.
            attach_to_parent(node_handle, handle);
        }
    }

    for (const auto [node, entity] : zip(nodes, new_entities))
    {
        const Handle handle = { registry, entity };
        if (not node.uuid.is_nil())
            entity_jobs.emplace_back(context.unpacker().unpack_any(node.uuid, handle));
    }

    co_await until_all_succeed(entity_jobs);
}


} // namespace josh
