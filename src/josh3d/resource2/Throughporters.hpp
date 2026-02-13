#pragma once
#include "ExternalScene.hpp"
#include "AsyncCradle.hpp"
#include "async/Coroutines.hpp"
#include "ECS.hpp"
#include "Filesystem.hpp"
#include "Processing.hpp"


/*
Throughporting refers to the act of loading an external asset directly into
the destination (ex. scene registry). It bypasses baking a ResourceFile, bookkeeping
in the ResourceDatabase and caching in the ResourceRegistry.

It's a 3-in-1 Import/Load/Unpack combo that works for simple cases where that is enough.

These are much less advanced and do not support full incremental streaming or
other fancy features. This is likely what a naive async implementation would do as well.
*/
namespace josh {

class MeshRegistry;
struct SkeletonStorage;
struct AnimationStorage;

struct ThroughportContext
{
    Registry&         registry;      // Destination for scene entities.
    MeshRegistry&     mesh_registry;
    SkeletonStorage&  skeleton_storage;
    AnimationStorage& animation_storage;
    AsyncCradleRef    async_cradle;
};

struct ESRThroughportParams
{
    bool          generate_mips = true; // FIXME: Completely ignored. Remove?
    Unitarization unitarization = Unitarization::InsertDummy; // Unitarization will alway be performed, but the algorithm can be customized.
};

/*
ExternalScene-based throughporter.

If `dst_entity` is not null, the scene(s) will be attached to it,
otherwise the scene will be emplaced directly into the `context.registry`.

Beware that the `scene` will likely contain ElementViews over external
data. Care must be taken to keep the data alive for at least as long
as the job itself. The simplest way to guarantee this is to emplace
the data owner into the `scene` as an entity component or a context entry.
*/
auto throughport_external_scene(
    esr::ExternalScene   scene,
    Entity               dst_entity,
    ESRThroughportParams params,
    ThroughportContext   context)
        -> Job<>;

struct GLTFThroughportParams
{
    bool          generate_mips = true;
    Unitarization unitarization = Unitarization::InsertDummy;
};

/*
cGLTF-based scene throughporter.
*/
auto throughport_scene_gltf(
    Path                  path,
    Entity                dst_entity,
    GLTFThroughportParams params,
    ThroughportContext    context)
        -> Job<>;

struct AssimpThroughportParams
{
    bool          generate_mips  = true;
    Unitarization unitarization  = Unitarization::InsertDummy;
    bool          collapse_graph = false;
    bool          merge_meshes   = false;
};

/*
Assimp-based scene throughporter.
*/
auto throughport_scene_assimp(
    Path                    path,
    Entity                  dst_entity,
    AssimpThroughportParams params,
    ThroughportContext      context)
        -> Job<>;


} // namespace josh
