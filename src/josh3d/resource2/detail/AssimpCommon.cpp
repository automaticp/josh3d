#include "AssimpCommon.hpp"
#include "Asset.hpp"
#include "ContainerUtils.hpp"
#include <assimp/BaseImporter.h>
#include <assimp/material.h>
#include <range/v3/numeric/accumulate.hpp>
#include <algorithm>


namespace josh::detail{


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

auto image_intent_to_ai_texture_type(ImageIntent intent, bool height_as_normals)
    -> aiTextureType
{
    switch (intent)
    {
        case ImageIntent::Albedo:    return aiTextureType_DIFFUSE;
        case ImageIntent::Specular:  return aiTextureType_SPECULAR;
        case ImageIntent::Normal:    return height_as_normals ? aiTextureType_HEIGHT : aiTextureType_NORMALS;
        case ImageIntent::Alpha:     return aiTextureType_OPACITY;
        case ImageIntent::Heightmap: return aiTextureType_DISPLACEMENT;
        case ImageIntent::Unknown:   return aiTextureType_UNKNOWN;
    }
    safe_unreachable("Invalid ImageIntent.");
}

// HMM: This makes me realize that the whole idea of "Intent" is useless,
// it should likely just be colorspace instead, maybe with extra parameters.
auto ai_texture_type_to_image_intent(aiTextureType textype, bool height_as_normals)
    -> ImageIntent
{
    switch (textype)
    {
        case aiTextureType_DIFFUSE:
        case aiTextureType_BASE_COLOR: return ImageIntent::Albedo;
        case aiTextureType_NORMALS:    return ImageIntent::Normal;
        case aiTextureType_HEIGHT:     return height_as_normals ? ImageIntent::Normal : ImageIntent::Heightmap;
        case aiTextureType_OPACITY:    return ImageIntent::Alpha;
        case aiTextureType_SPECULAR:   return ImageIntent::Specular;
        default:                       return ImageIntent::Unknown;
    }
}

auto get_ai_texture_type(const Path& path, ImageIntent intent)
    -> aiTextureType
{
    switch (intent)
    {
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

auto pack_mesh_elems(const aiMesh* ai_mesh)
    -> Vector<u32>
{
    auto faces = make_span(ai_mesh->mFaces, ai_mesh->mNumFaces);

    if (not faces.data()) throw AssetContentsParsingError("Mesh data does not contain Faces.");

    Vector<u32> elems_data; elems_data.reserve(3 * faces.size());

    for (const aiFace& ai_face : faces)
        for (const u32 index : make_span(ai_face.mIndices, ai_face.mNumIndices))
            elems_data.emplace_back(index);

    return elems_data;
}

auto pack_static_mesh_verts(const aiMesh* ai_mesh)
    -> Vector<VertexStatic>
{
    const auto positions  = make_span(ai_mesh->mVertices,         ai_mesh->mNumVertices);
    const auto uvs        = make_span(ai_mesh->mTextureCoords[0], ai_mesh->mNumVertices);
    const auto normals    = make_span(ai_mesh->mNormals,          ai_mesh->mNumVertices);
    const auto tangents   = make_span(ai_mesh->mTangents,         ai_mesh->mNumVertices);

    if (not uvs.data())        throw AssetContentsParsingError("Mesh data does not contain UVs.");
    if (not normals.data())    throw AssetContentsParsingError("Mesh data does not contain Normals.");
    if (not tangents.data())   throw AssetContentsParsingError("Mesh data does not contain Tangents.");

    Vector<VertexStatic> verts_data; verts_data.reserve(positions.size());

    for (const uindex i : irange(positions.size()))
    {
        const auto vert = VertexStatic::pack(
            v2v(positions[i]),
            v2v(uvs      [i]),
            v2v(normals  [i]),
            v2v(tangents [i])
        );
        verts_data.emplace_back(vert);
    }

    return verts_data;
}

auto pack_skinned_mesh_verts(const aiMesh* ai_mesh, Span<const u32> boneid2jointid)
    -> Vector<VertexSkinned>
{
    const auto positions  = make_span(ai_mesh->mVertices,         ai_mesh->mNumVertices);
    const auto uvs        = make_span(ai_mesh->mTextureCoords[0], ai_mesh->mNumVertices);
    const auto normals    = make_span(ai_mesh->mNormals,          ai_mesh->mNumVertices);
    const auto tangents   = make_span(ai_mesh->mTangents,         ai_mesh->mNumVertices);
    const auto bones      = make_span(ai_mesh->mBones,            ai_mesh->mNumBones   );

    if (not uvs.data())        throw AssetContentsParsingError("Mesh data does not contain UVs.");
    if (not normals.data())    throw AssetContentsParsingError("Mesh data does not contain Normals.");
    if (not tangents.data())   throw AssetContentsParsingError("Mesh data does not contain Tangents.");
    if (not bones.data())      throw AssetContentsParsingError("Mesh data does not contain Bones.");
    if (bones.size() > 255)    throw AssetContentsParsingError("Skeleton has to many Bones (>255).");

    // Info about influences and weights as pulled from assimp,
    // before convertion to a more "strict" packed internal format.
    using glm::uvec4;
    struct VertInfluence
    {
        vec4  ws {}; // Uncompressed weights.
        uvec4 ids{}; // Joint indices in the pre-order array. Refer to root node by default.
    };

    Vector<VertInfluence> vert_influences;
    vert_influences.resize(positions.size()); // Resize, not reserve.

    // NOTE: Use could use aiProcess_LimitBoneWeights to limit to 4 joint influences
    // per vertex, but we do not rely on that flag here and force 4 influences ourselves.
    // Assimp does not guarantee that weights are ordered by value, so we have to partition
    // it ourselves and take the top 4. Most of the time, however, the number of weights will
    // not be above 4, the partitioning won't be necessary and the buffer won't exceed the SBO size.
    SmallVector<aiVertexWeight, 4> weights;

    for (const auto [boneid, ai_bone] : enumerate(bones))
    {
        const u32 jointid = boneid2jointid[boneid];

        const auto ai_weights = make_span(ai_bone->mWeights, ai_bone->mNumWeights);
        const usize n = std::min<usize>(4, ai_weights.size());

        weights.clear();
        weights.insert(weights.begin(), ai_weights.begin(), ai_weights.end());
        std::ranges::nth_element(weights, weights.begin() + idiff(n), std::greater<>{}, &aiVertexWeight::mWeight);
        const auto top_weights = make_span(weights.data(), n);

        // Will have to renormalize ourselves.
        const float norm = ranges::accumulate(top_weights, 0.f, std::plus<>{}, &aiVertexWeight::mWeight);

        for (const auto [i, w] : enumerate(top_weights))
        {
            assert(i < 4);
            VertInfluence& influence = vert_influences[w.mVertexId];
            influence.ws [int(i)] = w.mWeight / norm;
            influence.ids[int(i)] = jointid;
        }
    }

    Vector<VertexSkinned> verts_data; verts_data.reserve(positions.size());

    for (const uindex i : irange(positions.size()))
    {
        const VertInfluence& influence = vert_influences[i];
        const auto vert = VertexSkinned::pack(
            v2v(positions [i]),
            v2v(uvs       [i]),
            v2v(normals   [i]),
            v2v(tangents  [i]),
            influence.ids,
            influence.ws
        );
        verts_data.emplace_back(vert);
    }

    return verts_data;
}


} // namespace josh::detail
