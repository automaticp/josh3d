#pragma once
#include "CategoryCasts.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLAPICore.hpp"
#include "GLAttributeTraits.hpp"
#include "GLBuffers.hpp"
#include "GLMutability.hpp"
#include "GLObjects.hpp"
#include "GLScalars.hpp"
#include "GLVertexArray.hpp"
#include "detail/StrongScalar.hpp"
#include <range/v3/view/enumerate.hpp>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <span>


namespace josh {


template<typename VertexT>
struct MeshID {
    uint64_t value{};
};


template<specializes_attribute_traits VertexT>
class MeshStorage {
public:
    using vertex_type = VertexT;
    using index_type  = GLuint;
    using id_type     = MeshID<vertex_type>;

    constexpr auto primitive_type() const noexcept -> Primitive   { return Primitive::Triangles; }
    constexpr auto element_type()   const noexcept -> ElementType { return ElementType::UInt;    }

    MeshStorage();

    void query(
        std::span<const id_type> ids,
        std::span<GLsizeiptr>    out_byte_offsets,
        std::span<GLsizei>       out_counts,
        std::span<GLint>         out_baseverts) const;

    // TODO:
    void query_indirect(
        std::span<const id_type>               ids,
        std::span<DrawElementsIndirectCommand> out_commands) const;

    void query_range(
        std::ranges::input_range         auto&& ids,
        std::output_iterator<GLsizeiptr> auto&& out_byte_offsets,
        std::output_iterator<GLsizei>    auto&& out_counts,
        std::output_iterator<GLint>      auto&& out_baseverts) const;

    // TODO:
    void query_range_indirect(
        std::ranges::input_range                          auto&& ids,
        std::output_iterator<DrawElementsIndirectCommand> auto&& out_commands) const;

    [[nodiscard]]
    auto insert(
        std::ranges::sized_range auto&& verts,
        std::ranges::sized_range auto&& indices)
            -> id_type;

    // Will generate indices for all verts by simply incrementing an integer.
    [[nodiscard]]
    auto insert(
        std::ranges::sized_range auto&& verts)
            -> id_type;

    [[nodiscard]]
    auto insert_buffer(
        RawBuffer<vertex_type, GLConst> verts,
        RawBuffer<index_type,  GLConst> indices)
            -> id_type;

    // Access the VAO that is bound to all of the mesh data.
    auto vertex_array() const noexcept
        -> RawVertexArray<GLConst>
    {
        return vao_;
    }

    // Access the buffer that holds all of the vertex data.
    auto vertex_buffer() const noexcept
        -> RawBuffer<vertex_type, GLConst>
    {
        return vbo_;
    }

    // Access the buffer that holds the index data for each mesh.
    auto index_buffer() const noexcept
        -> RawBuffer<index_type, GLConst>
    {
        return ebo_;
    }


    // Planned, but not trivial:
    //
    //   void remove(MeshID id);
    //   void compactify();
    //


private:
    UniqueVertexArray         vao_;
    UniqueBuffer<vertex_type> vbo_;
    NumElems                  vbo_size_ = 0; // Since allocations are amortized, size differs from capacity.
    NumElems                  vbo_cap_  = 0;
    UniqueBuffer<index_type>  ebo_;
    NumElems                  ebo_size_ = 0;
    NumElems                  ebo_cap_  = 0;
    double                    amortization_factor_ = 1.5;

    void reattach_vbo();
    void reattach_ebo();

    struct MeshPlacement {
        GLsizeiptr offset_bytes;
        GLsizei    count;
        GLint      basevert;
    };

    std::vector<MeshPlacement> table_;

};




template<specializes_attribute_traits VertexT>
MeshStorage<VertexT>::MeshStorage() {
    const AttributeIndex first_attrib{ 0 };
    const GLsizeiptr num_attribs = vao_->specify_custom_attributes<vertex_type>(first_attrib);

    for (AttributeIndex attrib_id{ 0 }; attrib_id < num_attribs; ++attrib_id) {
        vao_->enable_attribute(attrib_id);
        const VertexBufferSlot slot{ 0 }; // All the vertex data goes through the first buffer slot.
        vao_->associate_attribute_with_buffer_slot(attrib_id, slot);
    }
}


template<specializes_attribute_traits VertexT>
void MeshStorage<VertexT>::reattach_vbo() {
    const VertexBufferSlot slot  { 0 };
    const OffsetBytes      offset{ 0 };
    const StrideBytes      stride{ sizeof(vertex_type) };
    vao_->attach_vertex_buffer(slot, vbo_, offset, stride);
}


template<specializes_attribute_traits VertexT>
void MeshStorage<VertexT>::reattach_ebo() {
    vao_->attach_element_buffer(ebo_);
}


template<specializes_attribute_traits VertexT>
[[nodiscard]]
auto MeshStorage<VertexT>::insert(
    std::ranges::sized_range auto&& verts,
    std::ranges::sized_range auto&& indices)
        -> id_type
{
    const StoragePolicies policies{
        .mode        = StorageMode::StaticServer,
        .mapping     = PermittedMapping::ReadWrite,
        .persistence = PermittedPersistence::NotPersistent,
    };

    struct AppendResult {
        BufferRange appended_range;
        bool        was_resized;
    };

    auto append_to_buffer = [&, this]<typename T>(
        auto&&           input_range,
        UniqueBuffer<T>& buf,
        NumElems&        buf_size,
        NumElems&        buf_cap)
            -> AppendResult
    {
        const NumElems old_cap      = buf_cap;
        const NumElems old_size     = buf_size;
        const NumElems added_size   = std::ranges::ssize(input_range);
        const NumElems desired_size = old_size + added_size;
        const bool     needs_resize = desired_size > old_cap;

        if (needs_resize) {
            const NumElems amortized_size = GLsizeiptr(double(old_cap) * amortization_factor_);
            const NumElems new_size       = std::max(amortized_size, desired_size);

            UniqueBuffer<T> new_buf;
            new_buf->allocate_storage(new_size, policies);
            buf->copy_data_to(new_buf.get(), old_size);

            buf      = MOVE(new_buf);
            buf_cap  = new_size;
        }

        const BufferRange appended_range{ .offset = old_size.value, .count = added_size };
        const MappingWritePolicies policies{
            .previous_contents = PreviousContents::InvalidateMappedRange, // Is there a difference? We're accessing new memory anyway.
        };

        const std::span mapped = buf->map_range_for_write(appended_range, policies);
        do {
            std::ranges::copy(input_range, mapped.begin());
        } while (!buf->unmap_current());
        buf_size = desired_size;

        return { appended_range, needs_resize };
    };


    // Append to the VBO.
    const auto [vbo_range, vbo_resized] =
        append_to_buffer(FORWARD(verts), vbo_, vbo_size_, vbo_cap_);

    // Append to the EBO.
    const auto [ebo_range, ebo_resized] =
        append_to_buffer(FORWARD(indices), ebo_, ebo_size_, ebo_cap_);


    if (vbo_resized) { reattach_vbo(); }
    if (ebo_resized) { reattach_ebo(); }


    // Finally, push back the info about the struct into the lookup table and return the MeshID.

    const MeshPlacement mesh_placement{
        .offset_bytes = GLsizeiptr(ebo_range.offset * sizeof(index_type)),
        .count        = GLsizei   (ebo_range.count),
        .basevert     = GLint     (vbo_range.offset),
    };

    const auto new_id = MeshID<vertex_type>(table_.size());
    table_.push_back(mesh_placement);

    return new_id;
}


template<specializes_attribute_traits VertexT>
[[nodiscard]]
auto MeshStorage<VertexT>::insert(
    std::ranges::sized_range auto&& verts)
        -> id_type
{
    using std::views::iota;
    const NumElems num_verts = std::ranges::ssize(verts);
    auto indices = iota(index_type{}, num_verts);
    return insert(FORWARD(verts), indices);
}


template<specializes_attribute_traits VertexT>
[[nodiscard]]
auto MeshStorage<VertexT>::insert_buffer(
    RawBuffer<vertex_type, GLConst> verts,
    RawBuffer<index_type,  GLConst> indices)
        -> id_type
{
    const StoragePolicies policies{
        .mode        = StorageMode::StaticServer,
        .mapping     = PermittedMapping::ReadWrite,
        .persistence = PermittedPersistence::NotPersistent,
    };

    struct AppendResult {
        BufferRange appended_range;
        bool        was_resized;
    };

    auto append_to_buffer = [&, this]<typename T>(
        RawBuffer<T, GLConst> src_buf,
        UniqueBuffer<T>&      buf,
        NumElems&             buf_size,
        NumElems&             buf_cap)
            -> AppendResult
    {
        const NumElems old_cap      = buf_cap;
        const NumElems old_size     = buf_size;
        const NumElems added_size   = src_buf.get_num_elements();
        const NumElems desired_size = old_size + added_size;
        const bool     needs_resize = desired_size > old_cap;

        if (needs_resize) {
            const NumElems amortized_cap = GLsizeiptr(double(old_cap) * amortization_factor_);
            const NumElems new_cap       = std::max(amortized_cap, desired_size);

            UniqueBuffer<T> new_buf;
            new_buf->allocate_storage(new_cap, policies);
            buf->copy_data_to(new_buf.get(), old_size);

            buf      = MOVE(new_buf);
            buf_cap  = new_cap;
        }

        const OffsetElems src_offset = 0;
        const OffsetElems dst_offset = old_size.value;
        const BufferRange appended_range{ .offset = dst_offset, .count = added_size };

        // Do a server-side copy instead of mapping anything.
        src_buf.copy_data_to(buf.get(), added_size, src_offset, dst_offset);
        buf_size = desired_size;

        return { appended_range, needs_resize };
    };


    // Append to the VBO.
    const auto [vbo_range, vbo_resized] =
        append_to_buffer(verts, vbo_, vbo_size_, vbo_cap_);

    // Append to the EBO.
    const auto [ebo_range, ebo_resized] =
        append_to_buffer(indices, ebo_, ebo_size_, ebo_cap_);


    if (vbo_resized) { reattach_vbo(); }
    if (ebo_resized) { reattach_ebo(); }


    // Finally, push back the info about the struct into the lookup table and return the MeshID.

    const MeshPlacement mesh_placement{
        .offset_bytes = GLsizeiptr(ebo_range.offset * sizeof(index_type)),
        .count        = GLsizei   (ebo_range.count),
        .basevert     = GLint     (vbo_range.offset),
    };

    const auto new_id = MeshID<vertex_type>(table_.size());
    table_.push_back(mesh_placement);

    return new_id;
}




template<specializes_attribute_traits VertexT>
void MeshStorage<VertexT>::query(
    std::span<const id_type> ids,
    std::span<GLsizeiptr>    out_offsets_bytes,
    std::span<GLsizei>       out_counts,
    std::span<GLint>         out_baseverts) const
{
    assert(ids.size() == out_offsets_bytes.size());
    assert(ids.size() == out_counts.size());
    assert(ids.size() == out_baseverts.size());

    using ranges::views::enumerate;
    for (const auto [i, mesh_id] : enumerate(ids)) {
        assert(size_t(mesh_id.value) < table_.size());
        const MeshPlacement& placement = table_[size_t(mesh_id.value)];

        out_offsets_bytes[i] = placement.offset_bytes;
        out_counts       [i] = placement.count;
        out_baseverts    [i] = placement.basevert;
    }
}


template<specializes_attribute_traits VertexT>
void MeshStorage<VertexT>::query_range(
    std::ranges::input_range         auto&& ids,
    std::output_iterator<GLsizeiptr> auto&& out_offsets_bytes,
    std::output_iterator<GLsizei>    auto&& out_counts,
    std::output_iterator<GLint>      auto&& out_baseverts) const
{
    for (const id_type mesh_id : ids) {
        assert(size_t(mesh_id.value) < table_.size());
        const MeshPlacement& placement = table_[size_t(mesh_id.value)];

        *out_offsets_bytes++ = placement.offset_bytes;
        *out_counts++        = placement.count;
        *out_baseverts++     = placement.basevert;
    }
}




} // namespace josh
