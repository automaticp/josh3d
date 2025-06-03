#pragma once
#include "Common.hpp"
#include "Elements.hpp"
#include "VertexSkinned.hpp"
#include "VertexStatic.hpp"


namespace josh {

struct AttributeViews
{
    ElementsView indices;
    ElementsView positions;
    ElementsView uvs;
    ElementsView normals;
    ElementsView tangents;
    ElementsView joint_ids; // Only for skinned.
    ElementsView joint_ws;  // Only for skinned.
};

/*
POST: All attributes have required data, correct type, and are not sparse.
POST: Counts for each attribute match and equal `position.element_count`.
*/
void validate_attributes_static(const AttributeViews& a);

/*
POST: All attributes have required data, correct type, and are not sparse.
POST: Counts for each attribute match and equal `position.element_count`.
*/
void validate_attributes_skinned(const AttributeViews& a);

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



} // namespace josh
