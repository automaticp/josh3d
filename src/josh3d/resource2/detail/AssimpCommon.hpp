#pragma once
#include "../AssetImporter.hpp"
#include "Common.hpp"
#include "AABB.hpp"
#include "Asset.hpp"
#include "default/Resources.hpp"
#include "Scalars.hpp"
#include "Transform.hpp"
#include "UUID.hpp"
#include <assimp/aabb.h>
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/vector3.h>


/*
All of the Assimp-specific implementation details.

Because the whole implementation of scene importing is huge
we break it apart into multiple cpp files.
*/
namespace josh::detail {


using TextureIndex    = int32_t;
using MaterialIndex   = size_t;
using TextureJobIndex = size_t;


struct TextureInfo
{
    TextureIndex id     = -1;
    ImageIntent  intent = ImageIntent::Unknown;
};


struct MaterialIDs
{
    TextureIndex diffuse_id  = -1;
    TextureIndex specular_id = -1;
    TextureIndex normal_id   = -1;
};


struct MaterialUUIDs
{
    UUID diffuse_uuid;
    UUID specular_uuid;
    UUID normal_uuid;
};

// TODO: Deprecate
auto get_path_to_ai_texture(
    const Path&       parent_dir,
    const aiMaterial* material,
    aiTextureType     type)
        -> Path;

// TODO: Deprecate
auto get_ai_texture_type(const Path& path, ImageIntent intent)
    -> aiTextureType;

// NOTE: height_as_normals is needed for .obj, sadly.
auto image_intent_to_ai_texture_type(ImageIntent intent, bool height_as_normals = false)
    -> aiTextureType;

auto ai_texture_type_to_image_intent(aiTextureType textype, bool height_as_normals = false)
    -> ImageIntent;

auto pack_mesh_elems(const aiMesh* ai_mesh)
    -> Vector<u32>;

auto pack_static_mesh_verts(const aiMesh* ai_mesh)
    -> Vector<VertexStatic>;

auto pack_skinned_mesh_verts(const aiMesh* ai_mesh, Span<const u32> boneid2jointid)
        -> Vector<VertexSkinned>;

inline auto v2v(const aiVector3D& v) noexcept
    -> vec3
{
    return { v.x, v.y, v.z };
}

inline auto q2q(const aiQuaternion& q) noexcept
    -> quat
{
    return quat::wxyz(q.w, q.x, q.y, q.z);
}

inline auto m2m(const aiMatrix4x4& m) noexcept
    -> mat4
{
    // From assimp docs:
    //
    // "The transposition has nothing to do with a left-handed or right-handed
    // coordinate system  but 'converts' between row-major and column-major storage formats."
    auto msrc = m;
    msrc.Transpose();

    mat4 mdst;
    for (const usize i : irange(4))
        for (const usize j : irange(4))
            mdst[int(i)][int(j)] = msrc[i][j];

    return mdst;
}

// Decomposes the assimp Matrix4x4 into a regular transform.
inline auto m2tf(const aiMatrix4x4& m) noexcept
    -> Transform
{
    aiVector3D   pos;
    aiQuaternion rot;
    aiVector3D   sca;
    m.Decompose(sca, rot, pos);
    return { v2v(pos), q2q(rot), v2v(sca) };
}

inline auto s2sv(const aiString& s) noexcept
    -> StrView
{
    return { s.data, s.length };
}

inline auto s2s(const aiString& s)
    -> String
{
    return { s.data, s.length };
}

inline auto aabb2aabb(const aiAABB& aabb) noexcept
    -> LocalAABB
{
    return { v2v(aabb.mMin), v2v(aabb.mMax) };
}

auto import_skeleton_async(
    AssetImporterContext                         context,
    const aiNode*                                armature,
    HashMap<const aiNode*, size_t>&              node2jointid,
    const HashMap<const aiNode*, const aiBone*>& node2bone)
        -> Job<UUID>;

auto import_anim_async(
    AssetImporterContext                  context,
    const aiAnimation*                    ai_anim,
    const aiNode*                         armature,
    const UUID&                           skeleton_uuid,
    const HashMap<const aiNode*, size_t>& node2jointid)
        -> Job<UUID>;

auto import_static_mesh_async(
    AssetImporterContext context,
    const aiMesh*        ai_mesh)
        -> Job<UUID>;

auto import_skinned_mesh_async(
    AssetImporterContext                  context,
    const aiMesh*                         ai_mesh,
    UUID                                  skeleton_uuid,
    const HashMap<const aiNode*, size_t>& node2jointid)
        -> Job<UUID>;


auto import_material_async(
    AssetImporterContext context,
    String               name,
    MaterialUUIDs        texture_uuids,
    float                specpower)
        -> Job<UUID>;


auto import_mesh_entity_async(
    AssetImporterContext context,
    UUID                 mesh_uuid,
    UUID                 material_uuid,
    StrView              name)
        -> Job<UUID>;


auto import_scene_async(
    AssetImporterContext context,
    Path                 path,
    ImportSceneParams    params)
        -> Job<UUID>;


} // namespace josh::detail
