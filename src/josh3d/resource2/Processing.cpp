#include "Processing.hpp"
#include "Common.hpp"
#include "Elements.hpp"
#include "ExternalScene.hpp"
#include "GLObjectHelpers.hpp"
#include "MeshRegistry.hpp"
#include "Ranges.hpp"
#include "RuntimeError.hpp"
#include <glm/ext.hpp>
#include <limits>
#include <stb_image.h>


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

    make_available<Binding::ArrayBuffer>       (verts_staging->id());
    make_available<Binding::ElementArrayBuffer>(elems_staging->id());

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
