#include "SceneImporter.hpp"
#include "AssetUnpacker.hpp"
#include "AssetManager.hpp"
#include "ECS.hpp"
#include "LightCasters.hpp"
#include "Logging.hpp"
#include "ObjectLifecycle.hpp"
#include "RuntimeError.hpp"
#include "SceneGraph.hpp"
#include "ScopeExit.hpp"
#include "Tags.hpp"
#include "Transform.hpp"
#include "ContainerUtils.hpp"
#include "VPath.hpp"
#include "VirtualFilesystem.hpp"
#include "tags/ShadowCasting.hpp"
#include <entt/entity/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <range/v3/view/enumerate.hpp>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <string_view>


namespace josh {


using nlohmann::json;


namespace {


auto read_vec3(const json& j)
    -> glm::vec3
{
    glm::vec3 v{};
    if (j.size() != 3) {
        throw error::RuntimeError("Vector argument must be a three element array.");
    }
    for (size_t i{ 0 }; i < 3; ++i) {
        v[int(i)] = j.at(i).get<float>();
    }
    return v;
}


auto read_transform(const json& j)
    -> Transform
{
    Transform new_tf{};
    if (auto* j_tf = try_find(j, "transform")) {
        if (auto* pos = try_find(*j_tf, "position"))    { new_tf.position() = read_vec3(*pos);        }
        if (auto* ori = try_find(*j_tf, "orientation")) { new_tf.set_euler(radians(read_vec3(*ori))); }
        if (auto* sca = try_find(*j_tf, "scaling"))     { new_tf.scaling() = read_vec3(*sca);         }
    }
    return new_tf;
}


} // namespace


/*
Scene import can fail in multiple ways:

    - Fail to read/parse the json file
        - The caller should handle this, since they passed me the wrong file
        - Or the caller should be responisble for reading the file and parsing the json

    - Fail to read a particular entity during the initial pass
        - If we fail hard on any error:
            - We can do cleanup with `sweep_marked_*()` in a catch block
            - But already dispatched loading requests will be associated with dangling handles!
            - (!) We can attach the Futures to the handles directly to "discard" loads this way
            - But the loading process itself will still consume resources, even if the loaded assets are discarded
        - If we allow partial failures:
            - Any failure parsing a particular entity will skip that entitiy only
            - We can log on failure and expect the user to fix the error
            - Supporting programmatic recovery is a major PITA due to the current data flow
            - SceneGraph resolution will have to handle this somehow, skipping the
              invalid children and orphaning from failed parents
            - Might be worth keeping a set of failed entries in a side cache

    - Fail to import the asset of an entity after successfuly parsing the json
        - The caller of the unpacker should handle this

*/
void SceneImporter::import_from_json_file(const Path& json_file) {
    std::ifstream file{ json_file };
    const auto j = json::parse(file);
    import_from_json(j);
}


void SceneImporter::import_from_json(const json& j) {

    using ranges::views::enumerate;
    using error::RuntimeError;
    using fmt::format;

    // Positive - for user specified or serialized ids;
    // Negative - reserved for us to give unique ids for entries without them.
    using ID    = int64_t;

    // Index in the "entities" array.
    using Index = size_t;

    // Info about a single entry in the "entities" array.
    struct Entry {
        entt::entity entity = entt::null;
        Index        index  = -1;
    };


    thread_local std::unordered_map<ID, Entry> id2entry;
    ON_SCOPE_EXIT([&]{ id2entry.clear(); });


    // TODO: This is an interesting idea that can help with all
    // of the deferred initialization that can fail.
    auto create_uninitialized_handle = [&]() -> entt::handle {
        const entt::handle new_handle{ registry_, registry_.create() };
        mark_for_destruction(new_handle);
        return new_handle;
    };

    auto mark_initialized = [&](entt::handle h) {
        h.remove<MarkedForDestruction>();
    };

    auto sweep_uninitialized = [&]() {
        for (const auto entity : registry_.view<MarkedForDestruction>()) {
            const entt::handle handle{ registry_, entity };
            // TODO: Surely there must be a helper in SceneGraph.hpp :/
            if (has_parent(handle)) {
                detach_from_parent(handle);
            }
            if (has_children(handle)) {
                detach_all_children(handle);
            }
        }
        sweep_marked_for_destruction(registry_);
    };




    if (const auto* j_entities = try_find(j, "entities")) {

        // First, do a pre-pass on all entities and:
        // - Collect "id"s of the ones that have specify it
        // - Create "id"s based on index for those that don't
        // - Create handles for each "id" and store them into the map

        for (const auto& [index, j_entity] : enumerate(*j_entities)) {
            const auto  new_handle = create_uninitialized_handle();
            const Entry new_entry  = { new_handle.entity(), index };

            const ID new_id = [&]() -> ID {
                if (const auto* j_id = try_find(j_entity, "id")) {
                    try {
                        const auto id = j_id->get<ID>();
                        if (id < 0) {
                            throw RuntimeError(format("Entity \"id\" must be non-negative, got: {}.", id));
                        }
                        if (id2entry.contains(id)) {
                            throw RuntimeError(format("Duplicate \"id\": {}.", id));
                        }
                        return id;
                    } catch (const std::runtime_error& e) {
                        logstream() << format("Failed to establish \"id\" for entity {} - entity will be orphaned.", index) << ' ' << e.what() << '\n';
                        return -ID(index);
                    }
                } else {
                    return -ID(index);
                }
            }();

            id2entry.emplace(new_id, new_entry);
        }


        // Resolve the scene graph, leave entities as orphans on failure.
        for (const auto& [id, entry] : id2entry) {
            const auto& [entity, index] = entry;
            const entt::handle handle{ registry_, entity };
            const json& j_entity = (*j_entities).at(index);

            // Emplace transform.
            try {
                handle.emplace<Transform>(read_transform(j_entity)); // TODO: This fails the whole transform, but it probably shouldn't.
            } catch (const std::runtime_error& e) {
                logstream() << format("Failed to parse transform of entity {} - default will be used instead.", index) << ' ' << e.what() << '\n';
                handle.emplace_or_replace<Transform>();
            }

            // Emplace parent/child relationships.
            if (auto* j_children = try_find(j_entity, "children")) {
                for (const auto& [k, j_child] : enumerate(*j_children)) {
                    try {
                        const auto id = j_child.get<ID>();
                        if (id < 0) {
                            throw RuntimeError(format("Referenced child \"id\" must be non-negative, got: {}.", id));
                        }
                        if (const auto* item = try_find(id2entry, id)) {
                            const entt::handle child_handle{ registry_, item->second.entity };
                            if (has_parent(child_handle)) {
                                throw RuntimeError(format("Referenced child with \"id\": {} already has a parent.", id));
                            }
                            attach_child(handle, item->second.entity);
                        } else {
                            throw RuntimeError(format("No entity with \"id\": {}, but is referenced in the \"children\" list.", id));
                        }
                    } catch (const std::runtime_error& e) {
                        logstream() << format("Failed to resolve child {} of entity {}.", k, index) << ' ' << e.what() << '\n';
                    }
                }
            }
        }


        // This time resolve actual fields using importers.
        for (const auto& [id, entry] : id2entry) {
            const auto& [entity, index] = entry;
            const entt::handle handle{ registry_, entity };
            const json& j_entity = (*j_entities).at(index);

            if (const auto* j_type = try_find(j_entity, "type")) {
                try {
                    // NOTE: Might be worth enabling heterogeneous lookup and using views.
                    const auto type_name = j_type->get<std::string>();
                    if (auto* importer_item = try_find(type_importers_, type_name)) {
                        TypeImporter& importer = importer_item->second;

                        /*
                        What should *not* be handled by an importer:
                            - Creation of the new entity
                            - "type" - already verified
                            - "transform", "id", "children" - scene graph is resolved before

                        What *should* be handled by an importer:
                            - "vpath" and "path" - emplacing as components
                            - entity-specific fields (with reasonable defaults)
                            - throw an exception if importing cannot be done for some reason
                                - this will destroy the associated handle
                        */

                        // Emplace importer-specific properties.
                        // This is where assets will be submitted for loading too.
                        importer(j_entity, handle);

                    } else {
                        throw RuntimeError(format("No importer found for type \"{}\".", type_name));
                    }
                } catch (const std::runtime_error& e) {
                    logstream() << format("Failed import of entity {}.", index) << ' ' << e.what() << '\n';
                    continue; // Skip `mark_initialized()` call.
                }
            }
            // NOTE: If the type is not specified, then no importer is called and the
            // entity will likely just be classified as a "Node" or "GroupingNode".

            // We mark this as initialized for now, even though the async imports can still
            // fail later down the line. That will be discovered on unpacking and handled there.
            mark_initialized(handle);
        }
    } // if (const auto* j_entities = try_find(j, "entities"))

    // The handles that failed the import will keep their "uninitialized" mark here.
    // Destroy them as they are considered fully failed.
    sweep_uninitialized();
}




/*
Default importers for common scene objects.
*/
namespace {


auto get_asset_path(const json& j_entity)
    -> AssetPath
{
    auto* path  = try_find(j_entity, "path" );
    auto* vpath = try_find(j_entity, "vpath");

    if (path && vpath) {
        throw error::RuntimeError("Either \"path\" or \"vpath\" must be specified, not both.");
    }

    if (path) {
        return { path->get<Path>() };
    } else if (vpath) {
        const auto vp = VPath(vpath->get<Path>());
        return { vfs().resolve_path(vp) };
    } else {
        throw error::RuntimeError("External import needs \"path\" or \"vpath\" specified.");
    }
}




void import_model(
    AssetManager&  asset_manager,
    AssetUnpacker& asset_unpacker,
    const json&    j_entity,
    entt::handle   handle)
{
    AssetPath apath = get_asset_path(j_entity);
    auto job = asset_manager.load_model(MOVE(apath));
    asset_unpacker.submit_model_for_unpacking(handle.entity(), MOVE(job));
}


void import_skybox(
    AssetManager&  asset_manager,
    AssetUnpacker& asset_unpacker,
    const json&    j_entity,
    entt::handle   handle)
{
    AssetPath apath = get_asset_path(j_entity);
    auto job = asset_manager.load_cubemap(MOVE(apath), CubemapIntent::Skybox);
    asset_unpacker.submit_skybox_for_unpacking(handle.entity(), MOVE(job));
}


void import_point_light(const json& j_entity, entt::handle handle) {
    auto& plight = handle.emplace<PointLight>();
    if (auto* color = try_find(j_entity, "color")) {
        plight.color = read_vec3(*color);
    }
    if (auto* power = try_find(j_entity, "power")) {
        plight.power = power->get<float>();
    }
    if (auto* shadow = try_find(j_entity, "shadow")) {
        if (shadow->get<bool>()) {
            set_tag<ShadowCasting>(handle);
        }
    }
}


void import_directional_light(const json& j_entity, entt::handle handle) {
    auto& dlight = handle.emplace<DirectionalLight>();
    if (auto* color = try_find(j_entity, "color")) {
        dlight.color = read_vec3(*color);
    }
    if (auto* irradiance = try_find(j_entity, "irradiance")) {
        dlight.irradiance = irradiance->get<float>();
    }
    if (auto* shadow = try_find(j_entity, "shadow")) {
        if (shadow->get<bool>()) {
            set_tag<ShadowCasting>(handle);
        }
    }
}


void import_ambient_light(const json& j_entity, entt::handle handle) {
    auto& alight = handle.emplace<AmbientLight>();
    if (auto* color = try_find(j_entity, "color")) {
        alight.color = read_vec3(*color);
    }
    if (auto* irradiance = try_find(j_entity, "irradiance")) {
        alight.irradiance = irradiance->get<float>();
    }
}


} // namespace




void SceneImporter::register_importer(std::string_view type, TypeImporter importer) {
    type_importers_.emplace(std::string(type), std::move(importer));
}


SceneImporter::SceneImporter(
    AssetManager&   asset_manager,
    AssetUnpacker&  asset_unpacker,
    Registry&       registry
)
    : registry_{ registry }
{
    auto& am = asset_manager;
    auto& au = asset_unpacker;
    register_importer("Model",            [&](const json& j, entt::handle h) { import_model (am, au, j, h); });
    register_importer("Skybox",           [&](const json& j, entt::handle h) { import_skybox(am, au, j, h); });
    register_importer("PointLight",       &import_point_light      );
    register_importer("DirectionalLight", &import_directional_light);
    register_importer("AmbientLight",     &import_ambient_light    );
}


} // namespace josh
