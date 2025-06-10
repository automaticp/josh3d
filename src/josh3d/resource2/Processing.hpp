#pragma once
#include "AsyncCradle.hpp"
#include "Common.hpp"
#include "MeshRegistry.hpp"
#include "Scalars.hpp"
#include "Elements.hpp"
#include "ExternalScene.hpp"
#include "VertexSkinned.hpp"
#include "VertexStatic.hpp"


/*
Helpers for loading, convertsion and processing of all things related
to resources: Elements, Resources, Assets, etc. and their associated data.

This is a kitchen-sink header for all algorithms that can be reasonably reused.
Some parts might be separated later into dedicated files, or they might not.

The vocabulary (types) of these operations is likely defined elsewhere.
Here we try to tie this vocabulary together into a set of reusable algorithms.
*/
namespace josh {

struct EncodedImageInfo
{
    Extent2S resolution;
    u8       num_channels;
};

/*
Will use `stbi_info()` to get image info from a file.
Returns nullopt if the lookup failed for any reason.
*/
auto peek_encoded_image_info(const char* filepath)
    -> Optional<EncodedImageInfo>;

/*
Returns nullopt if the lookup failed for any reason.
Returns nullopt if `bytes.size() > INT_MAX`. Blame stbi.
*/
auto peek_encoded_image_info(Span<const ubyte> bytes)
    -> Optional<EncodedImageInfo>;

/*
POST: All attributes have required data and a correct type.
POST: Counts for each attribute match and equal `position.element_count`.
*/
void validate_attributes_static(const esr::MeshAttributes& a);

/*
POST: All attributes have required data and a correct type.
POST: Counts for each attribute match and equal `position.element_count`.
*/
void validate_attributes_skinned(const esr::MeshAttributes& a);

/*
PRE: View must be valid.
*/
auto pack_indices(const ElementsView& indices)
    -> Vector<u32>;

/*
PRE: Views must be valid. Their element counts should match.
*/
auto pack_attributes_static(
    const ElementsView& positions,
    const ElementsView& uvs,
    const ElementsView& normals,
    const ElementsView& tangents)
        -> Vector<VertexStatic>;

/*
PRE: Views must be valid. Their element counts should match.
*/
auto pack_attributes_skinned(
    const ElementsView& positions,
    const ElementsView& uvs,
    const ElementsView& normals,
    const ElementsView& tangents,
    const ElementsView& joint_ids,
    const ElementsView& joint_weights)
        -> Vector<VertexSkinned>;

/*
NOTE: This is more expensive than doing it directly on an array
of values, since we have to do conversions.

HMM: We could just provide a minmax helper in the Elements so that
it would do it on *source* data, before converting just 2 values.
*/
auto compute_aabb(const ElementsView& positions)
    -> Optional<LocalAABB>;

/*
Upload to staging buffers in the offscreen context then insert to storage in the local context.
*/
[[nodiscard]]
auto upload_static_mesh(
    Span<VertexStatic>  verts_data,
    Span<u32>           elems_data,
    MeshRegistry&       mesh_registry,
    AsyncCradleRef      async)
        -> Job<MeshID<VertexStatic>>;

/*
Upload to staging buffers in the offscreen context then insert to storage in the local context.
*/
[[nodiscard]]
auto upload_skinned_mesh(
    Span<VertexSkinned> verts_data,
    Span<u32>           elems_data,
    MeshRegistry&       mesh_registry,
    AsyncCradleRef      async)
        -> Job<MeshID<VertexSkinned>>;

enum class Unitarization
{
    InsertDummy,  // Create a dummy transform node that will hold the entities *and* their children.
    UnwrapToEdge, // Create a parent-child edge from the node in arbitrary order.
};
JOSH3D_DEFINE_ENUM_EXTRAS(Unitarization, InsertDummy, UnwrapToEdge);

/*
In ExternalScene representation each node can hold multiple entities at once:
multiple meshes, mesh+light, etc. This is rarely useful in practice.

Unitarization will duplicate the nodes to guarantee only one entity
per node in the newly constructed scene-graph.

POST: Each `esr::Node` in `scene` will have at most 1 entity in its entity list.
*/
void unitarize_external_scene(
    esr::ExternalScene& scene,
    Unitarization       algorithm);

} // namespace josh
