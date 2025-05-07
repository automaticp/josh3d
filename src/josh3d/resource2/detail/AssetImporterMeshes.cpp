#include "AssetImporter.hpp"
#include "VertexStatic.hpp"
#include "VertexSkinned.hpp"
#include <assimp/mesh.h>
#include <jsoncons/json.hpp>
#include <vector>


namespace josh::detail {


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
    std::span<VertexStatic> out_verts,
    const aiMesh*           ai_mesh)
{
    auto positions  = std::span(ai_mesh->mVertices,         ai_mesh->mNumVertices);
    auto uvs        = std::span(ai_mesh->mTextureCoords[0], ai_mesh->mNumVertices);
    auto normals    = std::span(ai_mesh->mNormals,          ai_mesh->mNumVertices);
    auto tangents   = std::span(ai_mesh->mTangents,         ai_mesh->mNumVertices);

    if (!normals.data())    { throw error::AssetContentsParsingError("Mesh data does not contain Normals.");    }
    if (!uvs.data())        { throw error::AssetContentsParsingError("Mesh data does not contain UVs.");        }
    if (!tangents.data())   { throw error::AssetContentsParsingError("Mesh data does not contain Tangents.");   }

    assert(out_verts.size() == positions.size());

    for (size_t i{ 0 }; i < out_verts.size(); ++i) {
        out_verts[i] = VertexStatic::pack(
            v2v(positions[i]),
            v2v(uvs      [i]),
            v2v(normals  [i]),
            v2v(tangents [i])
        );
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
    AssetImporterContext                             context,
    const aiMesh*                                    ai_mesh,
    UUID                                             skeleton_uuid,
    const std::unordered_map<const aiNode*, size_t>* node2jointid) // Optional, if Skinned.
        -> Job<UUID>
{
    co_await reschedule_to(context.thread_pool());

    const ResourcePathHint path_hint{
        .directory = "meshes",
        .name      = s2sv(ai_mesh->mName),
        .extension = "jmesh",
    };

    using VertexLayout = MeshFile::VertexLayout;
    using LODSpec      = MeshFile::LODSpec;
    using Compression  = MeshFile::Compression;

    const VertexLayout layout = [&]{
        if (ai_mesh->HasBones()) {
            return VertexLayout::Skinned;
        } else {
            return VertexLayout::Static;
        }
    }();

    auto vertex_size = [](VertexLayout layout) -> size_t {
        switch (layout) {
            case VertexLayout::Skinned: return sizeof(VertexSkinned);
            case VertexLayout::Static:  return sizeof(VertexStatic);
            default:                    return 0;
        }
    };

    const uint32_t num_verts   = ai_mesh->mNumVertices;
    const uint32_t num_elems   = 3 * ai_mesh->mNumFaces;
    const uint32_t elems_bytes = num_elems * sizeof(uint32_t);
    const uint32_t verts_bytes = num_verts * vertex_size(layout);

    LODSpec spec[1]{
        LODSpec{
            .num_verts   = num_verts,
            .num_elems   = num_elems,
            .verts_bytes = verts_bytes,
            .elems_bytes = elems_bytes,
            .compression = Compression::None,
            ._reserved0  = {},
        },
    };

    const MeshFile::Args args{
        .layout    = layout,
        .lod_specs = spec,
    };

    const size_t       file_size     = MeshFile::required_size(args);
    const ResourceType resource_type = MeshFile::resource_type;


    co_await reschedule_to(context.local_context());
    auto [uuid, mregion] = context.resource_database().generate_resource(resource_type, path_hint, file_size);
    co_await reschedule_to(context.thread_pool());



    MeshFile file = MeshFile::create_in(MOVE(mregion), uuid, args);

    file.skeleton_uuid() = skeleton_uuid;
    file.aabb()          = aabb2aabb(ai_mesh->mAABB);

    // NOTE: Ignoring compression for now. We have no compression options anyway.

    if (layout == VertexLayout::Skinned) {
        std::span<VertexSkinned> dst_verts = pun_span<VertexSkinned>(file.lod_verts_bytes(0));
        assert(node2jointid);
        extract_skinned_mesh_verts_to(dst_verts, ai_mesh, *node2jointid);
    } else if (layout == VertexLayout::Static) {
        std::span<VertexStatic> dst_verts = pun_span<VertexStatic>(file.lod_verts_bytes(0));
        extract_static_mesh_verts_to(dst_verts, ai_mesh);
    }

    std::span<uint32_t> dst_elems = pun_span<uint32_t>(file.lod_elems_bytes(0));
    extract_mesh_elems_to(dst_elems, ai_mesh);


    co_return uuid;
}


[[nodiscard]]
auto import_mesh_desc_async(
    AssetImporterContext  context,
    UUID                  mesh_uuid,
    std::string_view      name,
    MaterialUUIDs         mat_uuids)
        -> Job<UUID>
{
    co_await reschedule_to(context.thread_pool());
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

    const ResourcePathHint path_hint{
        .directory = "meshes",
        .name      = name,
        .extension = "jmdesc",
    };

    const auto resource_type = RT::MeshDesc;

    j["resource_type"] = resource_type.value();
    j["self_uuid"]     = serialize_uuid(UUID{}); // "Reserve space".

    std::string json_string;
    j.dump(json_string, jsoncons::indenting::indent);
    const size_t file_size = json_string.size();

    // After writing json to string (and learning the required size),
    // we go back to the resource database to generate actual files.
    co_await reschedule_to(context.local_context());
    auto [uuid, mregion] = context.resource_database().generate_resource(resource_type, path_hint, file_size);
    co_await reschedule_to(context.thread_pool());


    // FIXME: Dumb.
    j["self_uuid"] = serialize_uuid(uuid);
    json_string.clear();
    j.dump(json_string, jsoncons::indenting::indent);
    assert(file_size == json_string.size());


    // Finally, we can write the contents of the files through the mapped region.
    auto dst_bytes = std::span((std::byte*)mregion.get_address(), mregion.get_size());
    auto src_bytes = std::as_bytes(std::span(json_string));
    assert(src_bytes.size() == dst_bytes.size());
    std::ranges::copy(src_bytes, dst_bytes.begin());

    co_return uuid;
}


} // namespace josh::detail
