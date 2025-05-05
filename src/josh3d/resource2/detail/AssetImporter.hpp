#pragma once
#include "../AssetImporter.hpp"
#include "AABB.hpp"
#include "Math.hpp"
#include "Asset.hpp"
#include "Transform.hpp"
#include "UUID.hpp"
#include <assimp/quaternion.h>
#include <assimp/matrix4x4.h>
#include <assimp/vector3.h>
#include <assimp/aabb.h>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <string_view>
#include <unordered_map>
#include <span>


namespace josh {


class AssetImporter::Access {
public:
    auto& resource_database()  noexcept { return self_.resource_database_;  }
    auto& thread_pool()        noexcept { return self_.thread_pool_;        }
    auto& offscreen_context()  noexcept { return self_.offscreen_context_;  }
    auto& completion_context() noexcept { return self_.completion_context_; }
    auto& task_counter()       noexcept { return self_.task_counter_;       }
    auto& local_context()      noexcept { return self_.local_context_;      }
private:
    friend AssetImporter;
    Access(AssetImporter& self) : self_{ self } {}
    AssetImporter& self_;
};


} // namespace josh


namespace josh::detail {


using TextureIndex    = int32_t;
using MaterialIndex   = size_t;
using TextureJobIndex = size_t;


struct TextureInfo {
    TextureIndex id     = -1;
    ImageIntent  intent = ImageIntent::Unknown;
};


struct MaterialIDs {
    TextureIndex diffuse_id  = -1;
    TextureIndex specular_id = -1;
    TextureIndex normal_id   = -1;
};


struct MaterialUUIDs {
    UUID diffuse_uuid;
    UUID specular_uuid;
    UUID normal_uuid;
};


inline auto v2v(const aiVector3D& v) noexcept
    -> vec3
{
    return { v.x, v.y, v.z };
}


inline auto q2q(const aiQuaternion& q) noexcept
    -> quat
{
    return { q.w, q.x, q.y, q.z };
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
    for (size_t i{ 0 }; i < 4; ++i) {
        for (size_t j{ 0 }; j < 4; ++j) {
            mdst[int(i)][int(j)] = msrc[i][j];
        }
    }

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
    -> std::string_view
{
    return { s.data, s.length };
}


inline auto aabb2aabb(const aiAABB& aabb) noexcept
    -> LocalAABB
{
    return { v2v(aabb.mMin), v2v(aabb.mMax) };
}


template<typename DstT, typename SrcT>
auto pun_span(std::span<SrcT> src) noexcept
    -> std::span<DstT>
{
    assert((src.size_bytes()      % sizeof(DstT))  == 0);
    assert((uintptr_t(src.data()) % alignof(DstT)) == 0);
    return { std::launder(reinterpret_cast<DstT*>(src.data())), src.size_bytes() / sizeof(DstT) };
}


[[nodiscard]]
auto import_texture_async(
    AssetImporter::Access importer,
    Path                  src_filepath,
    ImportTextureParams   params)
        -> Job<UUID>;


[[nodiscard]]
auto import_skeleton_async(
    AssetImporter::Access                                   importer,
    const aiNode*                                           armature,
    std::unordered_map<const aiNode*, size_t>&              node2jointid,
    const std::unordered_map<const aiNode*, const aiBone*>& node2bone)
        -> Job<UUID>;


[[nodiscard]]
auto import_anim_async(
    AssetImporter::Access                            importer,
    const aiAnimation*                               ai_anim,
    const aiNode*                                    armature,
    const UUID&                                      skeleton_uuid,
    const std::unordered_map<const aiNode*, size_t>& node2jointid)
        -> Job<UUID>;


[[nodiscard]]
auto import_mesh_async(
    AssetImporter::Access                            importer,
    const aiMesh*                                    ai_mesh,
    UUID                                             skeleton_uuid,
    const std::unordered_map<const aiNode*, size_t>* node2jointid) // Optional, if Skinned.
        -> Job<UUID>;


[[nodiscard]]
auto import_mesh_desc_async(
    AssetImporter::Access importer,
    UUID                  mesh_uuid,
    std::string_view      name,
    MaterialUUIDs         mat_uuids)
        -> Job<UUID>;


[[nodiscard]]
auto import_model_async(
    AssetImporter::Access importer,
    Path                  path,
    ImportModelParams     params)
        -> Job<UUID>;


} // namespace josh::detail
