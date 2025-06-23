#pragma once
#include "AABB.hpp"
#include "Common.hpp"
#include "CommonMacros.hpp"
#include "Coroutines.hpp"
#include "ECS.hpp"
#include "EnumUtils.hpp"
#include "Filesystem.hpp"
#include "GLObjects.hpp"
#include "ImageProperties.hpp"
#include "LODPack.hpp"
#include "MeshStorage.hpp"
#include "Resource.hpp"
#include "SkeletalAnimation.hpp"
#include "Skeleton.hpp"
#include "Transform.hpp"
#include "UUID.hpp"
#include "VertexSkinned.hpp"
#include "VertexStatic.hpp"
#include <cstdint>
#include <memory>


/*
NOTE: Most of the resources must be simple reference types
without any kind of heavy data in them.
*/
namespace josh {

/*
HMM: What a given "resource" should maybe be able to do:

    Asset -> ResourceFile                 : Be imported from an asset to file
    UUID -> ResourceFile -> Resource      : Be loaded from disk
    Resource -> (Component...)            : Be emplaced into registry as components
    (Resource, Handle) -> (Component...)  : Be used to update components
    (Component...) -> Resource            : Be recreated from components (with a provoking component)
    Resource -> ResourceFile              : Be serialized back to a file
    ResourceFile -> Asset (+Metadata)     : Be optionally re-exported back to an asset

*/

/*
Defines a primary identifier for the resource and an associated type.
Namespace RT stands for ResourceType and is used to avoid collisions.
This macro won't work if you are in a namespace different from josh
because of the type trait. Oh, well...
*/
#define JOSH3D_DEFINE_RESOURCE_EXTRAS(Name, Type) \
    namespace RT { constexpr ResourceTypeHS Name = JOSH3D_CONCAT(#Name, _hs); } \
    template<> struct resource_traits<RT::Name> { using resource_type = Type; }


struct SceneResource
{
    struct Node
    {
        static constexpr int32_t no_parent = -1;
        Transform transform;
        int32_t   parent_index;
        UUID      uuid;
    };

    using node_list_type = Vector<Node>;
    std::shared_ptr<node_list_type> nodes; // Pre-order.
};
JOSH3D_DEFINE_RESOURCE_EXTRAS(Scene, SceneResource);

struct SkeletonResource
{
    std::shared_ptr<Skeleton> skeleton;
};
JOSH3D_DEFINE_RESOURCE_EXTRAS(Skeleton, SkeletonResource);

struct AnimationResource
{
    using keyframes_type = AnimationClip::JointKeyframes;
    std::shared_ptr<Vector<keyframes_type>> keyframes;
    double                                  duration_s;
    UUID                                    skeleton_uuid;
};
JOSH3D_DEFINE_RESOURCE_EXTRAS(Animation, AnimationResource);

struct StaticMeshResource
{
    LODPack<MeshID<VertexStatic>, 8> lods;
    LocalAABB                        aabb;
};
JOSH3D_DEFINE_RESOURCE_EXTRAS(StaticMesh, StaticMeshResource);

struct SkinnedMeshResource
{
    LODPack<MeshID<VertexSkinned>, 8> lods;
    LocalAABB                         aabb;
    UUID                              skeleton_uuid;
};
JOSH3D_DEFINE_RESOURCE_EXTRAS(SkinnedMesh, SkinnedMeshResource);

struct TextureResource
{
    SharedTexture2D texture;
};
JOSH3D_DEFINE_RESOURCE_EXTRAS(Texture, TextureResource);

struct MaterialResource
{
    UUID  diffuse_uuid; // Textures
    UUID  normal_uuid;
    UUID  specular_uuid;
    float specpower;
};
JOSH3D_DEFINE_RESOURCE_EXTRAS(Material, MaterialResource);

/*
TODO: Ultimately, this is a crappy stand-in for a more general "entity"
that can contain an arbitrary number of components by means of referencing
multiple UUIDs, possibly based on a prefab of some kind.
*/
struct MeshDescResource
{
    UUID mesh_uuid;
    UUID material_uuid;
};
JOSH3D_DEFINE_RESOURCE_EXTRAS(MeshDesc, MeshDescResource);

/*
Default resource metainfo like names and type.
*/

class ResourceInfo;

void register_default_resource_info(ResourceInfo& m);

/*
Default resource storage in the ResourceRegistry.
*/

class ResourceRegistry;

void register_default_resource_storage(ResourceRegistry& r);

/*
Loading of default resources from internal disk files.
This implicitly depends on the internal storage format of each resource.
*/

class ResourceLoader;
class ResourceLoaderContext;

void register_default_loaders(ResourceLoader& l);

auto load_static_mesh(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;

auto load_skinned_mesh(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;

auto load_mdesc(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;

auto load_material(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;

auto load_texture(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;

auto load_skeleton(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;

auto load_animation(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;

auto load_scene(
    ResourceLoaderContext context,
    UUID                  uuid)
        -> Job<>;

/*
Unpacking of default resources: delivery of fully or partially
loaded resources from the ResourceRegistry into the scene registry.
*/

class ResourceUnpacker;
class ResourceUnpackerContext;

void register_default_unpackers(ResourceUnpacker& u);

auto unpack_static_mesh(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;

auto unpack_skinned_mesh(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;

auto unpack_material(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;

auto unpack_mdesc(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;

auto unpack_scene(
    ResourceUnpackerContext context,
    UUID                    uuid,
    Handle                  handle)
        -> Job<>;

/*
Importing of external assets that correspond to default resources. This includes
loading from external files, conversion to the internal file format and bookkeeping
in the ResourceDatabase.
*/

class AssetImporter;
class AssetImporterContext;

void register_default_importers(AssetImporter& i);

/*
This is a separate enum from TextureFile::Encoding or
other similar types as this specifically selects *how*
imported image files are to be encoded.
*/
enum class ImportEncoding
{
    Raw,
    PNG,
    BC7,
};
JOSH3D_DEFINE_ENUM_EXTRAS(ImportEncoding, Raw, PNG, BC7);

struct ImportTextureParams
{
    // TODO: Only Raw and PNG is supported for now.
    // This should also specify Mixed mode at least.
    // So it probably needs a new enum.
    ImportEncoding encoding;
    Colorspace     colorspace;
    bool           generate_mips = true;
};

auto import_texture(
    AssetImporterContext context,
    Path                 path,
    ImportTextureParams  params)
        -> Job<UUID>;

struct ImportSceneParams
{
    ImportEncoding texture_encoding = ImportEncoding::PNG;
    bool generate_mips    = true;
    // bool skip_meshes     = false;
    // bool skip_textures   = false;
    // bool skip_skeletons  = false;
    // bool skip_animations = false;
    bool collapse_graph   = false; // Equivalent to aiProcess_OptimizeGraph
    bool merge_meshes     = false; // Equivalent to aiProcess_OptimizeMeshes
};

auto import_scene(
    AssetImporterContext context,
    Path                 path,
    ImportSceneParams    params)
        -> Job<UUID>;


} // namespace josh
