#pragma once
#include "AABB.hpp"
#include "Filesystem.hpp"
#include "GLBuffers.hpp"
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "CategoryCasts.hpp"
#include "MapMacro.hpp"
#include "MeshStorage.hpp"
#include "SkeletalAnimation.hpp"
#include "Skeleton.hpp"
#include "VertexPNUTB.hpp"
#include "VertexSkinned.hpp"
#include <compare>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>


namespace josh {


enum class AssetKind {
    Model,
    Mesh,
    SkinnedMesh,
    Skeleton,
    Animation,
    Texture,
    Cubemap,
};


/*
ImageIntent affects the number of channels
and the internal format of loaded textures.
*/
enum class ImageIntent {
    Unknown,
    Albedo,
    Alpha,
    Specular,
    Normal,
    Heightmap,
};


auto image_intent_minmax_channels(ImageIntent intent) noexcept
    -> std::pair<size_t, size_t>;


/*
CubemapIntent affects the channels and side
orientation of loaded cubemaps.
*/
enum class CubemapIntent {
    Unknown,
    Skybox,
};




/*
A canonical path to an asset on disk with an optional subpath
to identify file subresources. Immutable.

The intent is to make this hashable and "reliable" for caching.

Reliable is in quotes because we still have at least some unsolved problems:

    - Files modified after caching need to be checked for changes
      if full syncronization with state on disk is desired.

    - Case-insesitive filesystems (cough-cough, Windows...)
      can produce different paths to the same resource.
      Usually this is just a redundancy problem, not a correctness one.

TODO: This is a prime candidate for an interned string, given that
it's immutable and we copy it quite a bit when loading and returning assets.
*/
class AssetPath {
public:
    AssetPath() = default;

    AssetPath(const Path& entry)
        : entry_{ canonical(entry) }
    {}

    AssetPath(const Path& entry, std::string subpath)
        : entry_  { canonical(entry) }
        , subpath_{ MOVE(subpath)    }
    {}

    auto entry()   const noexcept -> const Path&      { return entry_;   }
    auto subpath() const noexcept -> std::string_view { return subpath_; }

    bool operator==(const AssetPath& other) const noexcept = default;
    std::strong_ordering operator<=>(const AssetPath& other) const noexcept = default;

private:
    Path        entry_;
    std::string subpath_;
};


} // namespace josh
namespace std {
template<> struct hash<josh::AssetPath> {
    std::size_t operator()(const josh::AssetPath& asset_path) const noexcept {
        // This is probably terrible, but...
        const auto phash = hash<josh::Path>{}(asset_path.entry());
        const auto shash = hash<std::string_view>{}(asset_path.subpath());
        return phash ^ (shash << 1);
        // I really don't care.
    }
};
} // namespace std
namespace josh {


// FIXME: GLMutable and GLConst are an accidental reuse, that does
// not make any sense for, say, skeletons or animations.
// This should just take bool as a second template param,
// and remap to mutability_tag via some kind of trait.
template<AssetKind KindV, mutability_tag MutT> struct Asset;
template<AssetKind KindV> using StoredAsset = Asset<KindV, GLMutable>;
template<AssetKind KindV> using SharedAsset = Asset<KindV, GLConst>;

using StoredTextureAsset     = StoredAsset<AssetKind::Texture>;
using StoredMeshAsset        = StoredAsset<AssetKind::Mesh>;
using StoredSkinnedMeshAsset = StoredAsset<AssetKind::SkinnedMesh>;
using StoredSkeletonAsset    = StoredAsset<AssetKind::Skeleton>;
using StoredAnimationAsset   = StoredAsset<AssetKind::Animation>;
using StoredModelAsset       = StoredAsset<AssetKind::Model>;
using StoredCubemapAsset     = StoredAsset<AssetKind::Cubemap>;

using SharedTextureAsset     = SharedAsset<AssetKind::Texture>;
using SharedMeshAsset        = SharedAsset<AssetKind::Mesh>;
using SharedSkinnedMeshAsset = SharedAsset<AssetKind::SkinnedMesh>;
using SharedSkeletonAsset    = SharedAsset<AssetKind::Skeleton>;
using SharedAnimationAsset   = SharedAsset<AssetKind::Animation>;
using SharedModelAsset       = SharedAsset<AssetKind::Model>;
using SharedCubemapAsset     = SharedAsset<AssetKind::Cubemap>;


#define JOSH3D_ASSET_CONVERSION_OP(KindV, ...) \
    operator Asset<KindV, GLConst>() const& { return { __VA_ARGS__ }; } \
    operator Asset<KindV, GLConst>() &&     { return { JOSH3D_MAP_LIST(MOVE, __VA_ARGS__) }; }


template<mutability_tag MutT>
struct Asset<AssetKind::Texture, MutT> {
    static constexpr AssetKind asset_kind = AssetKind::Texture;

    using mutability   = MutT;
    using texture_type = GLShared<RawTexture2D<MutT>>;

    AssetPath    path;
    ImageIntent  intent;
    texture_type texture;

    JOSH3D_ASSET_CONVERSION_OP(AssetKind::Texture, path, intent, texture)
};


template<mutability_tag MutT>
struct Asset<AssetKind::Mesh, MutT> {
    static constexpr AssetKind asset_kind = AssetKind::Mesh;

    using mutability         = MutT;
    using vertex_type        = VertexPNUTB;
    using index_type         = GLuint;
    using vertex_buffer_type = GLShared<RawBuffer<vertex_type, MutT>>;
    using index_buffer_type  = GLShared<RawBuffer<index_type,  MutT>>;

    AssetPath                         path;
    LocalAABB                         aabb;
    vertex_buffer_type                vertices;
    index_buffer_type                 indices;
    MeshID<vertex_type>               mesh_id; // Both mesh_id and separate buffers exist for now.
    std::optional<SharedTextureAsset> diffuse;
    std::optional<SharedTextureAsset> specular;
    std::optional<SharedTextureAsset> normal;

    JOSH3D_ASSET_CONVERSION_OP(AssetKind::Mesh, path, aabb, vertices, indices, mesh_id, diffuse, specular, normal)
};


template<mutability_tag MutT>
struct Asset<AssetKind::Skeleton, MutT> {
    static constexpr AssetKind asset_kind = AssetKind::Skeleton;

    using mutability    = MutT;
    using skeleton_type = std::conditional_t<gl_const<MutT>, const Skeleton, Skeleton>;

    std::shared_ptr<skeleton_type> skeleton;

    JOSH3D_ASSET_CONVERSION_OP(AssetKind::Skeleton, skeleton)
};


template<mutability_tag MutT>
struct Asset<AssetKind::Animation, MutT> {
    static constexpr AssetKind asset_kind = AssetKind::Animation;

    using mutability     = MutT;
    using animation_type = std::conditional_t<gl_const<MutT>, const SkeletalAnimation, SkeletalAnimation>;

    // AssetPath path;
    std::shared_ptr<animation_type> animation;

    JOSH3D_ASSET_CONVERSION_OP(AssetKind::Animation, animation)
};


template<mutability_tag MutT>
struct Asset<AssetKind::SkinnedMesh, MutT> {
    static constexpr AssetKind asset_kind = AssetKind::SkinnedMesh;

    using mutability         = MutT;
    using vertex_type        = VertexSkinned;
    using index_type         = GLuint;
    using vertex_buffer_type = GLShared<RawBuffer<vertex_type, MutT>>;
    using index_buffer_type  = GLShared<RawBuffer<index_type,  MutT>>;


    AssetPath                         path;
    LocalAABB                         aabb;
    vertex_buffer_type                vertices;
    index_buffer_type                 indices;
    MeshID<vertex_type>               mesh_id;
    SharedSkeletonAsset               skeleton_asset;
    std::vector<SharedAnimationAsset> animation_assets; // FIXME: Eventually, should be external.
    std::optional<SharedTextureAsset> diffuse;
    std::optional<SharedTextureAsset> specular;
    std::optional<SharedTextureAsset> normal;

    JOSH3D_ASSET_CONVERSION_OP(AssetKind::SkinnedMesh, path, aabb, vertices, indices, mesh_id, skeleton_asset, animation_assets, diffuse, specular, normal)
};


template<mutability_tag MutT>
struct Asset<AssetKind::Model, MutT> {
    static constexpr AssetKind asset_kind = AssetKind::Model;

    using mutability = MutT;
    using mesh_type  = std::variant<SharedMeshAsset, SharedSkinnedMeshAsset>;

    AssetPath              path;
    std::vector<mesh_type> meshes;

    JOSH3D_ASSET_CONVERSION_OP(AssetKind::Model, path, meshes)
};


template<mutability_tag MutT>
struct Asset<AssetKind::Cubemap, MutT> {
    static constexpr AssetKind asset_kind = AssetKind::Cubemap;

    using mutability   = MutT;
    using cubemap_type = GLShared<RawCubemap<MutT>>;

    AssetPath     path;
    CubemapIntent intent;
    cubemap_type  cubemap;

    JOSH3D_ASSET_CONVERSION_OP(AssetKind::Cubemap, path, intent, cubemap)
};




#undef JOSH3D_ASSET_CONVERSION_OP




namespace error {


class AssetLoadingError : public RuntimeError {
public:
    static constexpr auto prefix = "Asset Loading Error: ";
    AssetLoadingError(std::string msg)
        : AssetLoadingError(prefix, MOVE(msg))
    {}
protected:
    AssetLoadingError(const char* prefix, std::string msg)
        : RuntimeError(prefix, MOVE(msg))
    {}
};


class AssetFileImportFailure : public AssetLoadingError {
public:
    static constexpr auto prefix = "Asset File Import Failure: ";
    Path path;
    AssetFileImportFailure(Path path, std::string error_string)
        : AssetFileImportFailure(prefix, MOVE(path), MOVE(error_string))
    {}
protected:
    AssetFileImportFailure(const char* prefix, Path path, std::string error_string)
        : AssetLoadingError(prefix, MOVE(error_string))
        , path{ MOVE(path) }
    {}
};


class AssetContentsParsingError : public AssetLoadingError {
public:
    static constexpr auto prefix = "Asset Contents Parsing Error: ";
    AssetContentsParsingError(std::string msg)
        : AssetContentsParsingError(prefix, MOVE(msg))
    {}
protected:
    AssetContentsParsingError(const char* prefix, std::string msg)
        : AssetLoadingError(prefix, MOVE(msg))
    {}
};


} // namespace error




} // namespace josh
