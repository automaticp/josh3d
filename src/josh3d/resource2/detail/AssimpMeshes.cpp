#include "AssimpCommon.hpp"
#include "Common.hpp"
#include "Asset.hpp"
#include "AssetImporter.hpp"
#include "CoroCore.hpp"
#include "default/ResourceFiles.hpp"
#include "VertexFormats.hpp"
#include <assimp/mesh.h>
#include <jsoncons/json.hpp>


namespace josh::detail {
namespace {


void extract_skinned_mesh_verts_to(
    Span<VertexSkinned>                   out_verts,
    const aiMesh*                         ai_mesh,
    const HashMap<const aiNode*, size_t>& node2jointid)
{
    auto positions  = make_span(ai_mesh->mVertices,         ai_mesh->mNumVertices);
    auto uvs        = make_span(ai_mesh->mTextureCoords[0], ai_mesh->mNumVertices);
    auto normals    = make_span(ai_mesh->mNormals,          ai_mesh->mNumVertices);
    auto tangents   = make_span(ai_mesh->mTangents,         ai_mesh->mNumVertices);
    auto bones      = make_span(ai_mesh->mBones,            ai_mesh->mNumBones   );

    assert(out_verts.size() == positions.size());

    const bool valid_num_bones = bones.size() <= Skeleton::max_joints;

    if (!normals.data())    { throw AssetContentsParsingError("Mesh data does not contain Normals.");    }
    if (!uvs.data())        { throw AssetContentsParsingError("Mesh data does not contain UVs.");        }
    if (!tangents.data())   { throw AssetContentsParsingError("Mesh data does not contain Tangents.");   }
    if (!bones.data())      { throw AssetContentsParsingError("Mesh data does not contain Bones.");      }
    if (!valid_num_bones)   { throw AssetContentsParsingError("Armature has too many Bones (>255)."); }


    using glm::uvec4;
    // Info about weights as pulled from assimp,
    // before convertion to a more "strict" packed internal format.
    struct VertJointInfo {
        vec4   ws {}; // Uncompressed weights.
        uvec4  ids{}; // Refer to root node by default.
        int8_t n  {}; // Variable number of weights+ids. Because 4 is only an upper limit.
    };

    Vector<VertJointInfo> vert_joint_infos;
    vert_joint_infos.resize(positions.size()); // Resize, not reserve.

    // Now fill out the ids and weights for each vertex.
    for (const aiBone* bone : bones) {
        for (const aiVertexWeight& w : make_span(bone->mWeights, bone->mNumWeights)) {
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
    Span<VertexStatic> out_verts,
    const aiMesh*      ai_mesh)
{
    auto positions  = make_span(ai_mesh->mVertices,         ai_mesh->mNumVertices);
    auto uvs        = make_span(ai_mesh->mTextureCoords[0], ai_mesh->mNumVertices);
    auto normals    = make_span(ai_mesh->mNormals,          ai_mesh->mNumVertices);
    auto tangents   = make_span(ai_mesh->mTangents,         ai_mesh->mNumVertices);

    if (!normals.data())    { throw AssetContentsParsingError("Mesh data does not contain Normals.");    }
    if (!uvs.data())        { throw AssetContentsParsingError("Mesh data does not contain UVs.");        }
    if (!tangents.data())   { throw AssetContentsParsingError("Mesh data does not contain Tangents.");   }

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
    Span<uint32_t> out_elems,
    const aiMesh*  ai_mesh)
{
    auto faces = make_span(ai_mesh->mFaces, ai_mesh->mNumFaces);

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


} // namespace


auto import_static_mesh_async(
    AssetImporterContext context,
    const aiMesh*        ai_mesh)
        -> Job<UUID>
{
    co_await reschedule_to(context.thread_pool());

    const ResourcePathHint path_hint{
        .directory = "meshes",
        .name      = s2sv(ai_mesh->mName),
        .extension = "jmesh",
    };

    using LODSpec = StaticMeshFile::LODSpec;

    const uint32_t num_verts   = ai_mesh->mNumVertices;
    const uint32_t num_elems   = 3 * ai_mesh->mNumFaces;
    const uint32_t elems_bytes = num_elems * sizeof(uint32_t);
    const uint32_t verts_bytes = num_verts * sizeof(VertexStatic);

    // NOTE: Ignoring LODs for now.

    LODSpec spec[1]{
        LODSpec{
            .num_verts        = num_verts,
            .num_elems        = num_elems,
            .verts_size_bytes = verts_bytes,
            .elems_size_bytes = elems_bytes
        },
    };

    const StaticMeshFile::Args args{
        .lod_specs = spec,
    };

    const size_t       file_size     = StaticMeshFile::required_size(args);
    const ResourceType resource_type = StaticMeshFile::resource_type;

    auto [uuid, mregion] = context.resource_database().generate_resource(resource_type, path_hint, file_size);

    auto file = StaticMeshFile::create_in(MOVE(mregion), uuid, args);
    auto& header = file.header();

    header.aabb = aabb2aabb(ai_mesh->mAABB);

    extract_static_mesh_verts_to(pun_span<VertexStatic>(file.lod_verts_bytes(0)), ai_mesh);
    extract_mesh_elems_to       (pun_span<uint32_t>    (file.lod_elems_bytes(0)), ai_mesh);

    co_return uuid;
}


auto import_skinned_mesh_async(
    AssetImporterContext                  context,
    const aiMesh*                         ai_mesh,
    UUID                                  skeleton_uuid,
    const HashMap<const aiNode*, size_t>& node2jointid)
        -> Job<UUID>
{
    co_await reschedule_to(context.thread_pool());

    const ResourcePathHint path_hint{
        .directory = "meshes",
        .name      = s2sv(ai_mesh->mName),
        .extension = "jmesh",
    };

    using LODSpec = SkinnedMeshFile::LODSpec;

    const uint32_t num_verts   = ai_mesh->mNumVertices;
    const uint32_t num_elems   = 3 * ai_mesh->mNumFaces;
    const uint32_t elems_bytes = num_elems * sizeof(uint32_t);
    const uint32_t verts_bytes = num_verts * sizeof(VertexSkinned);

    // NOTE: Ignoring LODs for now.

    LODSpec spec[1]{
        LODSpec{
            .num_verts        = num_verts,
            .num_elems        = num_elems,
            .verts_size_bytes = verts_bytes,
            .elems_size_bytes = elems_bytes
        },
    };

    const SkinnedMeshFile::Args args{
        .skeleton_uuid = skeleton_uuid,
        .lod_specs     = spec,
    };

    const size_t       file_size     = SkinnedMeshFile::required_size(args);
    const ResourceType resource_type = SkinnedMeshFile::resource_type;

    auto [uuid, mregion] = context.resource_database().generate_resource(resource_type, path_hint, file_size);

    auto file = SkinnedMeshFile::create_in(MOVE(mregion), uuid, args);
    auto& header = file.header();

    header.aabb = aabb2aabb(ai_mesh->mAABB);

    extract_skinned_mesh_verts_to(pun_span<VertexSkinned>(file.lod_verts_bytes(0)), ai_mesh, node2jointid);
    extract_mesh_elems_to        (pun_span<uint32_t>     (file.lod_elems_bytes(0)), ai_mesh);

    co_return uuid;
}


} // namespace josh::detail
