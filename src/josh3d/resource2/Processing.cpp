#include "Processing.hpp"
#include "Common.hpp"
#include "Elements.hpp"
#include "ExternalScene.hpp"
#include "GLObjectHelpers.hpp"
#include "ImageData.hpp"
#include "MeshRegistry.hpp"
#include "Ranges.hpp"
#include "Errors.hpp"
#include "MallocSupport.hpp"
#include <glm/ext.hpp>
#include <stb_image.h>
#include <limits>


namespace josh {

auto peek_encoded_image_info(const char* filepath)
    -> Optional<EncodedImageInfo>
{
    int w, h, num_channels;
    const int ok = stbi_info(filepath, &w, &h, &num_channels);
    if (not ok) return nullopt;
    return EncodedImageInfo{
        .resolution   = Extent2S(w, h),
        .num_channels = u8(num_channels),
    };
}

auto peek_encoded_image_info(Span<const ubyte> bytes)
    -> Optional<EncodedImageInfo>
{
    if (bytes.size() > std::numeric_limits<int>::max())
        return nullopt;
    int w, h, num_channels;
    const int ok = stbi_info_from_memory(bytes.data(), int(bytes.size()), &w, &h, &num_channels);
    if (not ok) return nullopt;
    return EncodedImageInfo{
        .resolution   = Extent2S(w, h),
        .num_channels = u8(num_channels),
    };
}

auto decode_image_from_memory(Span<const ubyte> bytes, bool vflip)
    -> ImageData<ubyte>
{
    const usize max_size = std::numeric_limits<int>::max();

    if (bytes.size() > max_size)
        throw_fmt<RuntimeError>("Image byte buffer too large. Got {}, max is {}.",
            bytes.size(), max_size);

    const int size = int(bytes.size());

    int w, h, num_channels;
    stbi_set_flip_vertically_on_load_thread(vflip);
    auto image =
        unique_malloc_ptr<ubyte[]>(stbi_load_from_memory(bytes.data(), size, &w, &h, &num_channels, 0));

    if (not image)
        throw_fmt<RuntimeError>("Could not decode image with STBI: {}.", stbi_failure_reason());

    const Extent2S resoultion = { usize(w), usize(h) };

    return ImageData<ubyte>::take_ownership(MOVE(image), resoultion, usize(num_channels));
}

auto decode_image_from_memory(ElementsView src, bool vflip)
    -> ImageData<ubyte>
{
    // HMM: We *could* do an extra copy instead, but the "conversion"
    // of the byte data itself is meaningless as an operation.

    if (src.element != element_u8vec1)
        throw_fmt("Invalid element of encoded bytes. Expected u8vec1, got {}{}.",
            enum_string(src.element.type), enum_string(src.element.layout));

    if (src.stride != 1)
        throw_fmt("Invalid stride of encoded bytes. Expected 1, got {}.", src.stride);

    const usize size = element_size(src.element) * src.element_count;

    return decode_image_from_memory(make_span((const ubyte*)src.bytes, size), vflip);
}

auto load_image_from_file(const char* file, bool vflip)
    -> ImageData<ubyte>
{
    int w, h, num_channels;
    stbi_set_flip_vertically_on_load_thread(vflip);
    auto image =
        unique_malloc_ptr<ubyte[]>(stbi_load(file, &w, &h, &num_channels, 0));

    if (not image)
        throw_fmt("Could not load and decode image with STBI: {}.", stbi_failure_reason());

    const Extent2S resoultion = { usize(w), usize(h) };

    return ImageData<ubyte>::take_ownership(MOVE(image), resoultion, usize(num_channels));
}

/*
The "elements" in this case are pixels. Ex. `RGB == u8vec3`.
Will throw if a safe conversion cannot be made.

FIXME: This does a completely useless copy if the src.element.type is u8. Why is it like that?
We likely need an ImageView instead to describe raw image data.

PRE: `resolution.area() == pixels.element_count`.
*/
auto convert_raw_pixels_to_image_data(
    const ElementsView& pixels,
    const Extent2S&     resolution)
        -> ImageData<ubyte>
{
    assert(resolution.area() == pixels.element_count);

    const ElementsView& src          = pixels;
    const usize         num_channels = component_count(src.element.layout);

    auto result = ImageData<ubyte>(resolution, num_channels);

    const Element dst_element = {
        .type   = ComponentType::u8,
        .layout = ElementLayout(num_channels - 1), // FIXME: Not very safe.
    };

    const ElementsMutableView dst = {
        .bytes         = result.data(),
        .element_count = result.resolution().area(),
        .stride        = u32(element_size(dst_element)),
        .element       = dst_element,
    };

    if (not always_safely_convertible(src.element, dst.element))
        throw_fmt<RuntimeError>("Cannot guarantee safe conversion from {} to {}.",
            enum_string(src.element.type), enum_string(dst.element.type));

    copy_convert_elements(dst, src);

    return result;
}

auto upload_base_image_data(
    ImageView<const ubyte> imview,
    bool                   generate_mips)
        -> UniqueTexture2D
{
    UniqueTexture2D texture;

    const auto resolution   = imview.resolutioni();
    const auto num_levels   = generate_mips ? max_num_levels(resolution) : NumLevels(1);
    const auto num_channels = imview.num_channels();
    const auto iformat      = ubyte_iformat_from_num_channels(num_channels);
    texture->allocate_storage(resolution, iformat, num_levels);

    const auto pdtype   = PixelDataType::UByte;
    const auto pdformat = base_pdformat_from_num_channels(num_channels);
    texture->upload_image_region({ {}, resolution }, pdformat, pdtype, imview.data());

    if (generate_mips)
        texture->generate_mipmaps();

    return texture;
}

namespace {

/*
NOTE: We consider indices an attribute also.
*/
struct AttributeInfo
{
    ElementsView  view;
    const char*   name;
    Element       expected_element;
};

void validate_attribute(const AttributeInfo& info)
{
    if (not info.view)
        throw_fmt<RuntimeError>("No data for {} attribute.", info.name);

    if (not always_safely_convertible(info.view.element, info.expected_element))
        throw_fmt<RuntimeError>("Cannot safely convert attribute {} from {}{} to {}{}.",
            info.name,
            enum_string(info.view.element.type),     enum_string(info.view.element.layout),
            enum_string(info.expected_element.type), enum_string(info.expected_element.layout));
}

} // namespace

void validate_attributes_static(const esr::MeshAttributes& a)
{
    const AttributeInfo infos[] = {
        { a.indices,   "Index",    element_u32vec1 },
        { a.positions, "Position", element_f32vec3 },
        { a.uvs,       "UV",       element_f32vec2 },
        { a.normals,   "Normal",   element_f32vec3 },
        { a.tangents,  "Tangent",  element_f32vec3 },
    };

    for (const AttributeInfo& info : infos)
        validate_attribute(info);
}

void validate_attributes_skinned(const esr::MeshAttributes& a)
{
    validate_attributes_static(a);
    // NOTE: It is acceptable to have less than 4 joint influences.
    const AttributeInfo infos[] = {
        { a.joint_ids, "Joint Index",  element_u32vec4 },
        { a.joint_ws,  "Joint Weight", element_f32vec4 },
    };

    for (const AttributeInfo& info : infos)
        validate_attribute(info);
}

auto pack_attributes_static(
    const ElementsView& positions,
    const ElementsView& uvs,
    const ElementsView& normals,
    const ElementsView& tangents)
        -> Vector<VertexStatic>
{
    const usize vertex_count = positions.element_count;
    Vector<VertexStatic> verts; verts.reserve(vertex_count);

    // HMM: If we had all normalized conversions, *including*
    // normalized-to-normalized, we could do this with 4 calls
    // to copy_convert_elements(), which should hypothetically
    // be a little bit faster.

    for (const uindex i : irange(vertex_count))
    {
        const vec3 pos     = copy_convert_one_element<vec3>(positions, i);
        const vec2 uv      = copy_convert_one_element<vec2>(uvs,       i);
        const vec3 normal  = copy_convert_one_element<vec3>(normals,   i);
        const vec3 tangent = copy_convert_one_element<vec3>(tangents,  i);
        verts.push_back(VertexStatic::pack(pos, uv, normal, tangent));
    }

    return verts;
}

auto pack_attributes_skinned(
    const ElementsView& positions,
    const ElementsView& uvs,
    const ElementsView& normals,
    const ElementsView& tangents,
    const ElementsView& joint_ids,
    const ElementsView& joint_ws)
        -> Vector<VertexSkinned>
{
    const usize vertex_count = positions.element_count;
    Vector<VertexSkinned> verts; verts.reserve(vertex_count);

    for (const uindex i : irange(vertex_count))
    {
        const vec3  pos     = copy_convert_one_element<vec3> (positions, i);
        const vec2  uv      = copy_convert_one_element<vec2> (uvs,       i);
        const vec3  normal  = copy_convert_one_element<vec3> (normals,   i);
        const vec3  tangent = copy_convert_one_element<vec3> (tangents,  i);
        const uvec4 joints  = copy_convert_one_element<uvec4>(joint_ids, i);
        const vec4  joint_w = copy_convert_one_element<vec4> (joint_ws,  i);
        verts.push_back(VertexSkinned::pack(pos, uv, normal, tangent, joints, joint_w));
    }

    return verts;
}

auto pack_indices(const ElementsView& indices_view)
    -> Vector<u32>
{
    auto indices = Vector<u32>(indices_view.element_count);

    const ElementsMutableView dst = {
        .bytes         = indices.data(),
        .element_count = indices.size(),
        .stride        = sizeof(u32),
        .element       = element_u32vec1,
    };

    const usize written_count = copy_convert_elements(dst, indices_view);

    assert(indices_view.element_count == written_count);

    return indices;
}

auto compute_aabb(const ElementsView& positions)
    -> Optional<LocalAABB>
{
    if (not always_safely_convertible(positions.element, element_f32vec3))
        return nullopt;

    constexpr float inf = std::numeric_limits<float>::infinity();

    vec3 min = vec3(+inf);
    vec3 max = vec3(-inf);

    for (const uindex i : irange(positions.element_count))
    {
        const auto pos = copy_convert_one_element<vec3>(positions, i);
        for (const uindex _k : irange(3))
        {
            const int k = int(_k); // I hate glm for this.
            if (pos[k] < min[k]) min[k] = pos[k];
            if (pos[k] > max[k]) max[k] = pos[k];
        }
    }

    return LocalAABB{ min, max };
}

namespace {

template<typename VertexT>
auto upload_mesh(
    Span<VertexT>         verts_data,
    Span<u32>             elems_data,
    MeshRegistry&         mesh_registry,
    AsyncCradleRef        async)
        -> Job<MeshID<VertexT>>
{
    co_await reschedule_to(async.offscreen_context);

    const StoragePolicies policies = {
        .mode        = StorageMode::StaticServer,
        .mapping     = PermittedMapping::NoMapping,
        .persistence = PermittedPersistence::NotPersistent,
    };
    UniqueBuffer<VertexT> verts_staging = specify_buffer(to_span(verts_data), policies);
    UniqueBuffer<u32>     elems_staging = specify_buffer(to_span(elems_data), policies);

    co_await async.completion_context.until_ready_on(async.local_context, create_fence());

    glapi::make_available<Binding::ArrayBuffer>       (verts_staging->id());
    glapi::make_available<Binding::ElementArrayBuffer>(elems_staging->id());

    auto& mesh_storage = mesh_registry.ensure_storage_for<VertexT>();

    const MeshID<VertexT> mesh_id = mesh_storage.insert_buffer(verts_staging, elems_staging);

    co_return mesh_id;
}

} // namespace

auto upload_static_mesh(
    Span<VertexStatic>  verts_data,
    Span<u32>           elems_data,
    MeshRegistry&       mesh_registry,
    AsyncCradleRef      async)
        -> Job<MeshID<VertexStatic>>
{
    // Ja, allocate another frame, whatever.
    co_return co_await upload_mesh(verts_data, elems_data, mesh_registry, async);
}

auto upload_skinned_mesh(
    Span<VertexSkinned> verts_data,
    Span<u32>           elems_data,
    MeshRegistry&       mesh_registry,
    AsyncCradleRef      async)
        -> Job<MeshID<VertexSkinned>>
{
    co_return co_await upload_mesh(verts_data, elems_data, mesh_registry, async);
}

} // namespace josh
