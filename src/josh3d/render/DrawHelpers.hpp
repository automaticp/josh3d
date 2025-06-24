#pragma once
#include "Common.hpp"
#include "ECS.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICore.hpp"
#include "GLAPILimits.hpp"
#include "Materials.hpp"
#include "MeshStorage.hpp"
#include "Ranges.hpp"
#include "Scalars.hpp"
#include "UploadBuffer.hpp"


/*
Various helpers for organizing draw calls.
Mostly deals with MeshStorage, batching and MultiDraw/MDI.
*/
namespace josh {


/*
Returns a view of a sequence of `end - beg` numbers:
    `[beg, beg + 1, beg + 2, ..., end - 1]`

The view is sourced from a thread_local array and
can be invalidated by the next call to the same function.

This is used for batch-setting uniforms on the arrays,
in particular for sampler arrays with:
    `sp.set_uniform_intv(samplers_loc, size, data)`

The specified range has the same meaning as in `irange()`.

PRE: `end >= beg`.
*/
inline auto build_irange_tls_array(uindex beg, uindex end)
    -> Span<const i32>
{
    assert(end >= beg);
    thread_local uindex last_beg = 0;
    thread_local uindex last_end = 0;
    thread_local Vector<i32> values;
    if (beg != last_beg or end != last_end)
    {
        values.resize(end - beg);
        for (const uindex i : irange(beg, end))
            values[i] = i32(i);
    }
    return values;
}

/*
Equivalent to `build_irange_tls_array(0, n)`.
*/
inline auto build_irange_tls_array(usize n)
    -> Span<const i32>
{
    return build_irange_tls_array(0, n);
}

/*
Common when doing non-bindless batching.
*/
inline auto max_frag_texture_units()
    -> i32
{
    return glapi::get_limit(LimitI::MaxFragmentTextureImageUnits);
}

/*
Overrides the values if the entity has the respective material components,
otherwise, leaves the values as-is.

`inout_ids` are GL texture ids and are ordered: diffuse, specular, normal.

This is intended for the old material spec.
*/
inline void override_material(
    CHandle        handle,
    Array<u32, 3>& inout_ids,
    float&         inout_specpower)
{
    if (auto* mat = handle.try_get<MaterialDiffuse>())
        inout_ids[0] = mat->texture->id();

    if (auto* mat = handle.try_get<MaterialSpecular>())
    {
        inout_ids[1]    = mat->texture->id();
        inout_specpower = mat->shininess;
    }

    if (auto* mat = handle.try_get<MaterialNormal>())
        inout_ids[2] = mat->texture->id();
}

/*
Scratch for (direct) multidraw commands.
*/
struct MDScratch
{
    Vector<usize> offsets_bytes;
    Vector<i32>   counts;
    Vector<i32>   baseverts;
};

inline auto multidraw_tls_scratch()
    -> MDScratch&
{
    thread_local MDScratch md;
    md.offsets_bytes.clear();
    md.counts       .clear();
    md.baseverts    .clear();
    return md;
}

/*
Prepares draw parameters for a multidraw call and
executes it for each MeshID in the specified range.

NOTE: Mesa does not like *direct* multidraw.
*/
template<typename VertexT>
void multidraw_from_storage(
    const MeshStorage<VertexT>&         storage,
    BindToken<Binding::VertexArray>     bva,
    BindToken<Binding::Program>         bsp,
    BindToken<Binding::DrawFramebuffer> bfb,
    std::ranges::input_range auto&&     mesh_ids,
    MDScratch&                          md_scratch = multidraw_tls_scratch())
{
    assert(storage.vertex_array().id() == bva.id());
    md_scratch.offsets_bytes.clear();
    md_scratch.counts       .clear();
    md_scratch.baseverts    .clear();
    storage.query_range(
        mesh_ids,
        std::back_inserter(md_scratch.offsets_bytes),
        std::back_inserter(md_scratch.counts),
        std::back_inserter(md_scratch.baseverts)
    );
    glapi::multidraw_elements_basevertex(
        bva, bsp, bfb,
        storage.primitive_type(),
        storage.element_type(),
        md_scratch.offsets_bytes,
        md_scratch.counts,
        md_scratch.baseverts);
}

/*
TODO: Deprecate. Use the overload with explicit bva.
*/
template<typename VertexT>
void multidraw_from_storage(
    const MeshStorage<VertexT>&         storage,
    BindToken<Binding::Program>         bsp,
    BindToken<Binding::DrawFramebuffer> bfb,
    std::ranges::input_range auto&&     mesh_ids)
{
    const BindGuard bva = storage.vertex_array().bind();
    multidraw_from_storage(storage, bva, bsp, bfb, FORWARD(mesh_ids));
}

/*
Typedef for brevity.
*/
using MDICommand = DrawElementsIndirectCommand;

/*
Prepares draw parameters for an MDI call and
executes it for each MeshID in the specified range.

The `mdi_buffer` is restaged with the new commands.

NOTE: Mesa *likes* multidraw indirect.
PRE: `bva` must be refer to the `storage.vertex_array()`.
*/
template<typename VertexT>
void multidraw_indirect_from_storage(
    const MeshStorage<VertexT>&         storage,
    BindToken<Binding::VertexArray>     bva,
    BindToken<Binding::Program>         bsp,
    BindToken<Binding::DrawFramebuffer> bfb,
    std::ranges::input_range auto&&     mesh_ids,
    UploadBuffer<MDICommand>&           mdi_buffer)
{
    assert(storage.vertex_array().id() == bva.id());
    const auto get_cmd = [&](const MeshID<VertexT>& mesh_id)
        -> MDICommand
    {
        // TODO: The could likely be a batched version of this.
        return storage.query_one_indirect(mesh_id);
    };
    mdi_buffer.restage(mesh_ids | transform(get_cmd));
    if (mdi_buffer.num_staged() == 0) return;
    const BindGuard bmdi = mdi_buffer.bind_to_indirect_draw();
    glapi::multidraw_elements_indirect(
        bva, bsp, bfb, bmdi,
        storage.primitive_type(),
        storage.element_type(),
        i32(mdi_buffer.num_staged()),
        0, // Byte Offset
        0  // Byte Stride
    );
}

/*
PRE: `bva` must be refer to the `storage.vertex_array()`.
*/
template<typename VertexT>
void draw_one_from_storage(
    const MeshStorage<VertexT>&         storage,
    BindToken<Binding::VertexArray>     bva,
    BindToken<Binding::Program>         bsp,
    BindToken<Binding::DrawFramebuffer> bfb,
    MeshID<VertexT>                     mesh_id)
{
    assert(storage.vertex_array().id() == bva.id());
    const MeshPlacement p = storage.query_one(mesh_id);
    glapi::draw_elements_basevertex(
        bva, bsp, bfb,
        storage.primitive_type(),
        storage.element_type(),
        p.offset_bytes,
        p.count,
        p.basevert
    );
}



} // namespace josh
