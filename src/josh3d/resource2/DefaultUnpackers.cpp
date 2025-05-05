#include "Common.hpp"
#include "Components.hpp"
#include "Coroutines.hpp"
#include "DefaultResources.hpp"
#include "GLTextures.hpp"
#include "Materials.hpp"
#include "ResourceRegistry.hpp"
#include "ResourceUnpacker.hpp"
#include "ECS.hpp"
#include "SkinnedMesh.hpp"
#include "StaticMesh.hpp"
#include "Tags.hpp"
#include "tags/AlphaTested.hpp"
#include <boost/container/static_vector.hpp>
#include <cstdint>


namespace josh {


auto unpack_mesh(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>
{
    const auto task_guard = context.task_counter().obtain_task_guard();

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
    const auto aba_tag = uintptr_t(co_await peek_coroutine_address());

    /*
    FIXME: When we "bail", we likely want to report this somehow,
    maybe throw, maybe log, but something needs to be done to
    notify that unpacking was interrupted.
    */
    const auto bail = [](){};

    ResourceProgress progress;

    // Initial step.
    {
        auto [resource, usage] = co_await context.resource_registry().get_resource<RT::Mesh>(uuid, &progress);
        co_await reschedule_to(context.local_context());

        if (!handle.valid())
            co_return bail();

        if (const auto* static_mesh = get_if<MeshResource::Static>(&resource.mesh)) {
            if (has_component<StaticMesh>(handle))
                co_return bail();

            insert_component<StaticMesh>(handle, {
                .lods    = static_mesh->lods,
                .usage   = MOVE(usage),
                .aba_tag = aba_tag
            });

        }

        if (const auto* skinned_mesh = get_if<MeshResource::Skinned>(&resource.mesh)) {
            if (has_component<SkinnedMe2h>(handle))
                co_return bail();

            insert_component<SkinnedMe2h>(handle, {
                .lods           = skinned_mesh->lods,
                .usage          = MOVE(usage),
                .skeleton       = skinned_mesh->skeleton.resource.skeleton,
                .skeleton_usage = skinned_mesh->skeleton.usage,
                .aba_tag        = aba_tag,
            });

        }
    }

    // Incremental updates.
    while (progress != ResourceProgress::Complete) {
        auto [resource, usage] = co_await context.resource_registry().get_resource<RT::Mesh>(uuid, &progress);
        co_await reschedule_to(context.local_context());

        if (!handle.valid())
            co_return bail();

        if (const auto* static_mesh = get_if<MeshResource::Static>(&resource.mesh)) {
            if (!has_component<StaticMesh>(handle))
                co_return bail();

            auto& component = handle.get<StaticMesh>();
            if (component.aba_tag != aba_tag)
                co_return bail();

            component.lods = static_mesh->lods;
            // TODO: Should we update the usage too? Why would it change?
        }

        if (const auto* skinned_mesh = get_if<MeshResource::Skinned>(&resource.mesh)) {
            if (!has_component<SkinnedMe2h>(handle))
                co_return bail();

            auto& component = handle.get<SkinnedMe2h>();
            if (component.aba_tag != aba_tag)
                co_return bail();

            component.lods = skinned_mesh->lods;
        }

    }
}


auto unpack_material_diffuse(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>
{
    const auto task_guard = context.task_counter().obtain_task_guard();

    const auto aba_tag = uintptr_t(co_await peek_coroutine_address());
    const auto bail = [](){};

    ResourceProgress progress;
    {
        auto [resource, usage] = co_await context.resource_registry().get_resource<RT::Texture>(uuid, &progress);
        co_await reschedule_to(context.local_context());

        if (!handle.valid())
            co_return bail();

        if (has_component<MaterialDiffuse>(handle))
            co_return bail();

        if (resource.texture->get_component_type<PixelComponent::Alpha>()
            != PixelComponentType::None)
        {
            set_tag<AlphaTested>(handle);
        }

        insert_component<MaterialDiffuse>(handle, {
            .texture = MOVE(resource.texture),
            .usage   = MOVE(usage),
            .aba_tag = aba_tag,
        });
    }

    while (progress != ResourceProgress::Complete) {
        auto [resource, usage] = co_await context.resource_registry().get_resource<RT::Texture>(uuid, &progress);
        co_await reschedule_to(context.local_context());

        if (!handle.valid())
            co_return bail();

        if (!has_component<MaterialDiffuse>(handle))
            co_return bail();

        auto& component = handle.get<MaterialDiffuse>();
        if (component.aba_tag != aba_tag)
            co_return bail();

        component.texture = MOVE(resource.texture);
    }
}


auto unpack_material_normal(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>
{
    const auto task_guard = context.task_counter().obtain_task_guard();

    const auto aba_tag = uintptr_t(co_await peek_coroutine_address());
    const auto bail = [](){};

    ResourceProgress progress;
    {
        auto [resource, usage] = co_await context.resource_registry().get_resource<RT::Texture>(uuid, &progress);
        co_await reschedule_to(context.local_context());

        if (!handle.valid())
            co_return bail();

        if (has_component<MaterialNormal>(handle))
            co_return bail();

        insert_component<MaterialNormal>(handle, {
            .texture = MOVE(resource.texture),
            .usage   = MOVE(usage),
            .aba_tag = aba_tag,
        });
    }

    while (progress != ResourceProgress::Complete) {
        auto [resource, usage] = co_await context.resource_registry().get_resource<RT::Texture>(uuid, &progress);
        co_await reschedule_to(context.local_context());

        if (!handle.valid())
            co_return bail();

        if (!has_component<MaterialNormal>(handle))
            co_return bail();

        auto& component = handle.get<MaterialNormal>();
        if (component.aba_tag != aba_tag)
            co_return bail();

        component.texture = MOVE(resource.texture);
    }
}


auto unpack_material_specular(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>
{
    const auto task_guard = context.task_counter().obtain_task_guard();

    const auto aba_tag = uintptr_t(co_await peek_coroutine_address());
    const auto bail = [](){};

    // FIXME: We have to way to get this if all we are loading is a Texture.
    // We should likely be loading a material instead.
    const float specpower = 128.f;

    ResourceProgress progress;
    {
        auto [resource, usage] = co_await context.resource_registry().get_resource<RT::Texture>(uuid, &progress);
        co_await reschedule_to(context.local_context());

        if (!handle.valid())
            co_return bail();

        if (has_component<MaterialSpecular>(handle))
            co_return bail();

        insert_component<MaterialSpecular>(handle, {
            .texture   = MOVE(resource.texture),
            .usage     = MOVE(usage),
            .shininess = specpower,
            .aba_tag   = aba_tag,
        });
    }

    while (progress != ResourceProgress::Complete) {
        auto [resource, usage] = co_await context.resource_registry().get_resource<RT::Texture>(uuid, &progress);
        co_await reschedule_to(context.local_context());

        if (!handle.valid())
            co_return bail();

        if (!has_component<MaterialSpecular>(handle))
            co_return bail();

        auto& component = handle.get<MaterialSpecular>();
        if (component.aba_tag != aba_tag)
            co_return bail();

        component.texture = MOVE(resource.texture);
    }
}


auto unpack_mdesc(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>
{
    const auto task_guard = context.task_counter().obtain_task_guard();

    auto [mdesc, usage] = co_await context.resource_registry().get_resource<RT::MeshDesc>(uuid);

    StaticVector<Job<>, 4> jobs;

    // FIXME: We should not unpack directly, and instead go through the unpack dispatch table.
    // There should be an interface similar to get_resource<>(), but unpack_resource<>().

    jobs.emplace_back(unpack_mesh(context, mdesc.mesh_uuid, handle));

    if (!mdesc.diffuse_uuid.is_nil()) {
        jobs.emplace_back(unpack_material_diffuse(context, mdesc.diffuse_uuid, handle));
    }

    if (!mdesc.normal_uuid.is_nil()) {
        jobs.emplace_back(unpack_material_normal(context, mdesc.normal_uuid, handle));
    }

    if (!mdesc.specular_uuid.is_nil()) {
        jobs.emplace_back(
            // FIXME: Dirty hack to compensate for inability to load specpower.
            [&]() -> Job<> {
                co_await unpack_material_specular(context, mdesc.specular_uuid, handle);
                handle.get<MaterialSpecular>().shininess = mdesc.specpower;
            }()
        );
    }

    co_await context.completion_context().until_all_ready(jobs);
}


} // namespace josh
