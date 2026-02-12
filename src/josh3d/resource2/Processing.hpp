#pragma once
#include "AsyncCradle.hpp"
#include "Common.hpp"
#include "ImageData.hpp"
#include "MeshRegistry.hpp"
#include "Scalars.hpp"
#include "Elements.hpp"
#include "ExternalScene.hpp"
#include "VertexFormats.hpp"


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
Will throw on failure.
*/
auto decode_image_from_memory(Span<const ubyte> bytes, bool vflip = true)
    -> ImageData<ubyte>;

/*
Will throw on failure.
FIXME: These preconditions are odd. What's the point of this overload then?
PRE: `src.element == element_u8vec1`.
PRE: `src.stride == 1`.
*/
auto decode_image_from_memory(ElementsView src, bool vflip = true)
    -> ImageData<ubyte>;

/*
Will throw on failure.
*/
auto load_image_from_file(const char* file, bool vflip = true)
    -> ImageData<ubyte>;

/*
The "elements" in this case are pixels. Ex. `RGB <-> u8vec3`.
Will throw if a safe conversion cannot be made.

FIXME: This does a completely useless copy if the src.element.type is u8. Why is it like that?
We likely need an ImageView instead to describe raw image data.

PRE: `resolution.area() == pixels.element_count`.
*/
auto convert_raw_pixels_to_image_data(
    const ElementsView& pixels,
    const Extent2S&     resolution)
        -> ImageData<ubyte>;

/*
This will simply pick one of the R/RG/RGB/RGBA internal formats,
and *might* generate mips, but won't set any sampling or swizzle.

You might want to create separate texture views from this base
texture with their own samplers, swizzle and colorspace.

PRE: This must be called from a GPU context.
*/
auto upload_base_image_data(
    ImageView<const ubyte> imview,
    bool                   generate_mips = true)
        -> UniqueTexture2D;

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

/*
Will pick what to do depending on the data location and format in `image`.
FIXME: I really don't like this base_dir parameter. Couldn't we just resolve it?
*/
auto load_or_decode_esr_image(
    const esr::Image& image,
    const Path&       base_dir)
    -> ImageData<ubyte>;

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
