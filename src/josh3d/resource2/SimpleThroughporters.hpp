#pragma once
#include "AsyncCradle.hpp"
#include "Coroutines.hpp"
#include "ECS.hpp"
#include "Filesystem.hpp"


/*
Throughporting refers to the act of loading an external asset directly into
the destination (ex. scene registry). It bypasses baking a ResourceFile, bookkeeping
in the ResourceDatabase and caching in the ResourceRegistry.

It's a 3-in-1 Import/Load/Unpack combo that works for simple cases where that is enough.

These are much less advanced and do not support proper streaming or
other fancy features. This is likely what a naive implementation would do as well.
*/
namespace josh {


class MeshRegistry;

struct AssimpThroughportParams
{
    bool generate_mips  = true;
    bool collapse_graph = false;
    bool merge_meshes   = false;
};

/*
Assimp-based scene throughporter.
*/
auto throughport_scene_assimp(
    Path                    path,
    Handle                  dst_handle,
    AssimpThroughportParams params,
    AsyncCradleRef          async_cradle,
    MeshRegistry&           mesh_registry)
        -> Job<>;


struct GLTFThroughportParams
{
    bool generate_mips = true;
};

/*
cGLTF-based scene throughporter.
*/
auto throughport_scene_gltf(
    Path                  path,
    Handle                dst_handle,
    GLTFThroughportParams params,
    AsyncCradleRef        async_cradle,
    MeshRegistry&         mesh_registry)
        -> Job<>;


} // namespace josh
