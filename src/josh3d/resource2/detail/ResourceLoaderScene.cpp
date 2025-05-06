#include "Camera.hpp"
#include "ContainerUtils.hpp"
#include "Coroutines.hpp"
#include "DefaultResources.hpp"
#include "ECS.hpp"
#include "FileMapping.hpp"
#include "LightCasters.hpp"
#include "Ranges.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceLoader.hpp"
#include "ResourceRegistry.hpp"
#include "Resource.hpp"
#include "RuntimeError.hpp"
#include "SceneGraph.hpp"
#include "UUID.hpp"
#include <cstdint>
#include <jsoncons/basic_json.hpp>


namespace josh::detail {
namespace {


using jsoncons::json;
using Node = SceneResource::Node;


auto emplace_scene_into_handle(
    ResourceLoader::Access loader,
    Handle                 dst_handle,
    const SceneResource&   scene)
        -> Job<>
{
    co_await reschedule_to(loader.local_context());

    /*
    TODO
    */

}


struct NodeInfo {
    int32_t num_children = 0;
    int32_t last_child   = -1; // Last and prev instead of frist and next
    int32_t prev_sibling = -1; // so that the storage order would be preserved for siblings.
};


auto read_vec3(const json& j)
    -> vec3
{
    vec3 v{};
    if (j.size() != 3) { throw error::RuntimeError("Vector argument must be a three element array."); }
    v[0] = j[0].as<float>();
    v[1] = j[1].as<float>();
    v[2] = j[2].as<float>();
    return v;
}


auto read_quat(const json& j)
    -> quat
{
    quat q{};
    if (j.size() != 4) { throw error::RuntimeError("Quaternion argument must be a four element array."); }
    q[0] = j[0].as<float>();
    q[1] = j[1].as<float>();
    q[2] = j[2].as<float>();
    q[3] = j[3].as<float>();
    return q;
}


auto read_transform(const json& j)
    -> Transform
{
    Transform new_tf{};
    if (const auto& j_tf = j.at_or_null("transform"); !j_tf.is_null()) {
        if (const auto& j_pos = j_tf.at_or_null("position"); !j_pos.is_null()) { new_tf.position()    = read_vec3(j_pos); }
        if (const auto& j_rot = j_tf.at_or_null("rotation"); !j_rot.is_null()) { new_tf.orientation() = read_quat(j_rot); }
        if (const auto& j_sca = j_tf.at_or_null("scaling");  !j_sca.is_null()) { new_tf.scaling()     = read_vec3(j_sca); }
    }
    return new_tf;
}

#if 0
auto read_object(const json& j)
    -> Node::any_type
{
    const auto& type = j.at_or_null("type");

    if (type.is_null()) {
        return {}; // Default-construct to an Empty state.
    }

    const auto type_sv = type.as_string_view();

    // TODO: A lookup table, lol.
    if (type_sv == "Mesh") { // FIXME: This is not a mesh, but mdesc. Fix the serializer.
        UUID mdesc_uuid = deserialize_uuid(j.at("mdesc_uuid").as_string_view());
        return ResourceItem{ .type = RT::MeshDesc, .uuid = mdesc_uuid };
    } else if (type_sv == "OrthographicCamera") { // TODO: Not exported currently
        const auto params = OrthographicCamera::Params{
            .width  = j.at("width").as<float>(),
            .height = j.at("height").as<float>(),
            .z_near = j.at("z_near").as<float>(),
            .z_far  = j.at("z_far").as<float>(),
        };
        return std::make_unique<OrthographicCamera>(params);
    } else if (type_sv == "PerspectiveCamera") {
        const auto params = PerspectiveCamera::Params{
            .fovy_rad     = j.at("fovy_rad").as<float>(),
            .aspect_ratio = j.at("aspect_ratio").as<float>(),
            .z_near       = j.at("z_near").as<float>(),
            .z_far        = j.at("z_far").as<float>(),
        };
        return std::make_unique<PerspectiveCamera>(params);
    } else if (type_sv == "DirectionalLight") {
        return DirectionalLight{
            .color      = read_vec3(j.at("color")),
            .irradiance = j.at("irradiance").as<float>(),
        };
    } else if (type_sv == "PointLight") {
        return PointLight{
            .color = read_vec3(j.at("color")),
            .power = j.at("power").as<float>(),
        };
    }
    throw error::RuntimeError("Unrecognized scene node type.");
}
#endif

void populate_nodes_preorder(
    std::vector<Node>&        dst_nodes,
    int32_t                   dst_parent_idx,
    int32_t                   src_current_idx,
    std::span<const NodeInfo> infos,
    const jsoncons::json&     entities_array)
{
    const int32_t dst_current_idx = int32_t(dst_nodes.size());
    const auto& entity = entities_array[src_current_idx];

    dst_nodes.emplace_back(Node{
        .transform    = read_transform(entity),
        .parent_index = dst_parent_idx,
        .uuid = {} //
    });

    // Then iterate children.
    int32_t src_child_idx = infos[src_current_idx].last_child;
    while (src_child_idx != -1) {
        populate_nodes_preorder(
            dst_nodes,
            dst_current_idx,
            src_child_idx,
            infos,
            entities_array
        );
        src_child_idx = infos[src_child_idx].prev_sibling;
    }
}


auto load_scene_from_disc(
    ResourceLoader::Access loader,
    UUID                   uuid)
        -> SceneResource
{
    // TODO: Mapping can fail. Sad.
    MappedRegion mregion = loader.resource_database().try_map_resource(uuid);

    using Node = SceneResource::Node;
    const jsoncons::json j = jsoncons::json::parse((char*)mregion.get_address(), (char*)mregion.get_address() + mregion.get_size());
    const auto& entities = j.at("entities");

    auto entities_array = entities.array_range();

    // Reconstruct pre-order.
    //
    // NOTE: IDK if I should even bother with this, but this is to guarantee
    // that the array is indeed stored in pre-order, which we might rely on.
    //
    // For emplacing into the scene this does not matter, but might come up
    // in other usecases.

    thread_local std::vector<NodeInfo> infos; infos.resize(entities.size(), {});
    thread_local std::vector<int32_t>  roots; roots.clear();

    auto read_parent_idx = [](const jsoncons::json& j) {
        return j.get_value_or<int32_t>("parent", -1);
    };

    // TODO: Check parent ids for being in-range.

    for (const auto [i, entity] : enumerate(entities_array)) {
        // Parent index in the json *source* array.
        const int32_t parent_idx = read_parent_idx(entity);
        if (parent_idx == -1) {
            roots.emplace_back(i);
        } else {
            NodeInfo& parent = infos[parent_idx];
            NodeInfo& node   = infos[i];
            // Maybe this is right, IDK.
            if (parent.last_child != -1) {
                node.prev_sibling = parent.last_child;
                infos[parent.last_child].prev_sibling = int32_t(i);
            }
            ++parent.num_children;
            parent.last_child = int32_t(i);
        }
    }

    std::vector<Node> nodes; nodes.reserve(entities.size());

    for (const int32_t root_idx : roots) {
        populate_nodes_preorder(
            nodes,
            Node::no_parent,
            root_idx,
            infos,
            entities
        );
    }

    return SceneResource(std::make_shared<std::vector<Node>>(MOVE(nodes)));
}


auto load_and_emplace_mdesc(
    ResourceLoader::Access loader,
    UUID                   uuid,
    Handle                 dst_handle)
        -> Job<void>
{
    co_await reschedule_to(loader.thread_pool());

    auto [mdesc, usage] =
        co_await loader.resource_registry().get_resource<RT::MeshDesc>(uuid);



}


auto load_and_emplace_resource_async(
    ResourceLoader::Access loader,
    ResourceItem           item,
    Handle                 dst_handle)
        -> Job<void>
{
    const auto task_guard = loader.task_counter().obtain_task_guard();

    // FIXME: Should go through a dispatch table.
    switch (item.type) {
        case RT::MeshDesc:
            return load_and_emplace_mdesc(loader, item.uuid, dst_handle);
        default:
            // FIXME: This is a logic error, technically.
            throw error::RuntimeError("Resource type cannot be emplaced into a scene.");
    };
}


} // namespace

#if 0
auto load_scene_async(
    ResourceLoader::Access loader,
    UUID                   uuid,
    Handle                 dst_handle)
        -> Job<void>
{
    const auto task_guard = loader.task_counter().obtain_task_guard();
    co_await reschedule_to(loader.thread_pool());

    /*
    1. Try to get live scene.
        - If not live and not pending, load it yourself.
        - If not live, but pending, suspend until loaded. // How?
    2. Emplace the scene "structure" in the scene registry.
    3. For each resource in the scene:
        - Try to get live resource.
            - If not live and not pending, start a loading task.
            - If not live, but pending, start a waiting task.
            - If live, keep a "usage" reference to emplace later.
        - Emplace each thing that becomes ready in a when_any style. // How?

    */

    GetOutcome outcome;
    std::optional<PublicResource<ResourceType::Scene>> scene_opt =
        co_await loader.resource_registry().get_if_cached_or_join_pending<ResourceType::Scene>(uuid, outcome);

    if (outcome == GetOutcome::WasPending) {
        // Each pending job will be resumed on the same thread. So get out of the way asap.
        co_await reschedule_to(loader.thread_pool());
    } else if (outcome == GetOutcome::NeedsLoading) {
        auto scene_resource = load_scene_from_disc(loader, uuid);
        scene_opt = loader.resource_registry().cache_and_resolve_pending<ResourceType::Scene>(uuid, MOVE(scene_resource));
    }

    // Past this point, the scene must be available one way or the other.
    assert(scene_opt);
    const PublicResource<ResourceType::Scene>& scene = *scene_opt;
    const std::span<const Node> nodes = *scene.resource.nodes;

    // We are going to start loading resources from the scene,
    // as well as emplacing it into the registry.
    auto& registry = loader.scene_registry();

    std::vector<Entity> new_entities; new_entities.resize(nodes.size());
    std::vector<Job<void>> all_jobs;  all_jobs.reserve(1 + new_entities.size()); // One extra for the scene graph itself.

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

    auto create_scene_graph_in_registry = [&]() -> Job<void> {
        co_await reschedule_to(loader.local_context());
        registry.create(new_entities.begin(), new_entities.end());

        for (const auto [node, entity] : zip(nodes, new_entities)) {
            const Handle handle = { registry, entity };
            handle.emplace<Transform>(node.transform);
            if (node.parent_index != Node::no_parent) {
                attach_to_parent(handle, new_entities[node.parent_index]);
            }
        }
    };

    auto emplace_per_object_components = [&](const Node::object_type& object, const Entity& entity)
        -> Job<void>
    {
        // Both `object` and `entity` are persistent here until the end of the outer function
        // so they are okay to capture or pass as arguments by reference. Local variables are not though.

        // NOTE: Customization point.
        //
        // There is technically only 2 types of behaviour here:
        //   - Emplacing data directly;
        //   - Loading and emplacing a resource.
        //
        const overloaded object_visitor {
            [](const Node::Empty&) -> Job<void> {
                co_return; // This being a coroutine is a waste.
            },
            [&](const ResourceItem& item) -> Job<void> {
                return load_and_emplace_resource_async(loader, item, Handle{ registry, entity });
            },
            [&](const std::unique_ptr<PerspectiveCamera>& camera) -> Job<void> {
                co_await reschedule_to(loader.local_context());
                loader.scene_registry().emplace<PerspectiveCamera>(entity, *camera);
            },
            [&](const std::unique_ptr<OrthographicCamera>& camera) -> Job<void> {
                co_await reschedule_to(loader.local_context());
                loader.scene_registry().emplace<OrthographicCamera>(entity, *camera);
            },
            [&](const DirectionalLight& dlight) -> Job<void> {
                co_await reschedule_to(loader.local_context());
                loader.scene_registry().emplace<DirectionalLight>(entity, dlight);
            },
            [&](const PointLight& plight) -> Job<void> {
                co_await reschedule_to(loader.local_context());
                loader.scene_registry().emplace<PointLight>(entity, plight);
            },
        };

        return visit(object_visitor, object);
    };

    // The local context is a queue, by emplacing the scene_graph job
    // before any others, the happens-before in this thread will impose
    // the same onto the local context.
    //
    // Meaning, the per-object jobs will always execute *after* their
    // entities have been assigned, and it's safe to access entities
    // in the `new_entities` array as well as their Transform and
    // AsParent/AsChild components.
    //
    // However, the order of completion *between* the per-object
    // jobs is not guaranteed in any way, and they should not
    // rely on any other components being *complete*.

    all_jobs.emplace_back(create_scene_graph_in_registry());
    for (const auto [node, entity] : zip(nodes, new_entities)) {
        all_jobs.emplace_back(emplace_per_object_components(node.object, entity));
    }

    co_await loader.completion_context().until_all_ready(all_jobs);
    co_return;
}
#endif



} // namespace josh::detail
