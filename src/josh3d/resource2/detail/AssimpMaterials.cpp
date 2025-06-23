#include "AssimpCommon.hpp"
#include "AssetImporter.hpp"
#include "Coroutines.hpp"
#include "default/Resources.hpp"
#include "ResourceDatabase.hpp"
#include "UUID.hpp"
#include <assimp/types.h>
#include <jsoncons/basic_json.hpp>


namespace josh::detail {


using jsoncons::json;


auto import_material_async(
    AssetImporterContext context,
    String               name,
    MaterialUUIDs        texture_uuids,
    float                specpower)
        -> Job<UUID>
{
    co_await reschedule_to(context.thread_pool());

    /*
    Simple json spec:

    {
        "diffuse": "f3f2e850-b5d4-11ef-ac7e-96584d5248b2",
        "normal": "1d07af07-eafc-48e5-a618-30722b576dc6",
        "specular": "1d07af07-eafc-48e5-a618-30722b576dc6",
        "specpower": 128.0
    }

    Null UUID means no material texture.
    */

    json j;
    j["diffuse"]   = serialize_uuid(texture_uuids.diffuse_uuid);
    j["normal"]    = serialize_uuid(texture_uuids.normal_uuid);
    j["specular"]  = serialize_uuid(texture_uuids.specular_uuid);
    j["specpower"] = specpower;

    const ResourcePathHint path_hint = {
        .directory = "materials",
        .name      = name,
        .extension = "jmatl"
    };

    // We have to write the json to string first to find out the
    // actual size of the file so that we could request a new resource.

    String json_string;
    j.dump_pretty(json_string);

    const size_t file_size = json_string.size();

    auto [uuid, mregion] = context.resource_database().generate_resource(RT::Material, path_hint, file_size);

    // Finally, we can write the contents of the files through the mapped region.
    const auto dst_bytes = to_span(mregion);
    const auto src_bytes = as_bytes(to_span(json_string));
    assert(src_bytes.size() == dst_bytes.size());
    std::ranges::copy(src_bytes, dst_bytes.begin());

    co_return uuid;
}


auto import_mesh_entity_async(
    AssetImporterContext  context,
    UUID                  mesh_uuid,
    UUID                  material_uuid,
    StrView               name)
        -> Job<UUID>
{
    co_await reschedule_to(context.thread_pool());

    /*
    Simple json spec for the time being:

    {
        "mesh": "f3f2e850-b5d4-11ef-ac7e-96584d5248b2",
        "material": "1d07af07-eafc-48e5-a618-30722b576dc6",
    }
    */

    // We will construct json as text first, serialize to a string,
    // then request the resource file from the database at a later point.

    json j;
    j["mesh"]     = serialize_uuid(mesh_uuid);
    j["material"] = serialize_uuid(material_uuid);

    const ResourcePathHint path_hint{
        .directory = "meshes",
        .name      = name,
        .extension = "jmdesc",
    };

    const auto resource_type = RT::MeshDesc;

    // FIXME: Writing self-references is interesting, but maybe should only
    // be considered later once we figure out how we encode all this data anyway.
    // I'll keep this here for now, but it's more of an oddity.

    j["resource_type"] = resource_type.value();
    j["self_uuid"]     = serialize_uuid(UUID{}); // "Reserve space".

    String json_string;
    j.dump_pretty(json_string);

    const size_t file_size = json_string.size();

    // After writing json to string (and learning the required size),
    // we go back to the resource database to generate actual files.
    auto [uuid, mregion] = context.resource_database().generate_resource(resource_type, path_hint, file_size);

    j["self_uuid"] = serialize_uuid(uuid);
    json_string.clear();
    j.dump_pretty(json_string);

    assert(file_size == json_string.size());

    // Finally, we can write the contents of the files through the mapped region.
    auto dst_bytes = to_span(mregion);
    auto src_bytes = as_bytes(to_span(json_string));
    assert(src_bytes.size() == dst_bytes.size());
    std::ranges::copy(src_bytes, dst_bytes.begin());

    co_return uuid;
}


} // namespace josh::detail
