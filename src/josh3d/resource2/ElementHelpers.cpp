#include "ElementHelpers.hpp"
#include "Common.hpp"
#include "Elements.hpp"
#include "Ranges.hpp"
#include "RuntimeError.hpp"


namespace josh {
namespace {

/*
NOTE: We consider indices as an attribute also.
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

void validate_attributes_static(const AttributeViews& a)
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

void validate_attributes_skinned(const AttributeViews& a)
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



} // namespace josh
