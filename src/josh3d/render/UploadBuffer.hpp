#pragma once
#include "CommonConcepts.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLMutability.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include <cassert>
#include <iterator>
#include <ranges>


namespace josh {

// TODO: Should be vocabulary in gl.
struct BufferRange {
    OffsetElems offset;
    NumElems    count;
};


/*
Buffer wrapper for a particular usage pattern:

    1. Data is uploaded to the GPU buffer periodically, likely every frame;
    2. Data is only ever transferred CPU -> GPU, never read back;
    3. Data is an array of structs, but not extremely large.
       Small enough for the CPU side to overwrite the whole buffer each frame.
    4. The amount of uploaded data is not immediately predictable,
       likely because it needs to undergo some filtering (culling) before uploading.

Overall, this finds use in drawing multiple lights, instancing, bindless resources, etc.

NOTE: Don't yell "BUT WHAT ABOUT PERSISTENT MAPPING" at me, this works fine as is.
*/
template<trivially_copyable T>
class UploadBuffer {
public:
    UploadBuffer() = default;

    // Clear staged storage.
    void clear();

    // Get the number of staged elements in storage.
    auto num_staged() const noexcept -> NumElems { return NumElems{ staged_.size() }; }

    // Clear staged storage and stage new data.
    auto restage(std::ranges::range auto&& r) -> BufferRange;

    // Stage new data by appending to the existing staged storage.
    auto stage(std::ranges::range auto&& r) -> BufferRange;

    // Commit all data in staged storage to the GPU.
    void ensure_synced();

    // Is the GPU data the same as the staged storage?
    bool is_synced() const noexcept { return is_synced_; }

    // Commit staged data to the GPU and bind the buffer to the `index`.
    auto bind_to_ssbo_index(GLuint index)
        -> BindToken<BindingIndexed::ShaderStorageBuffer>;

    // Commit staged data to the GPU and bind the buffer `range` to the `index`.
    auto bind_range_to_ssbo_index(const BufferRange& range, GLuint index)
        -> BindToken<BindingIndexed::ShaderStorageBuffer>;

private:
    std::vector<T>  staged_;
    UniqueBuffer<T> buffer_;
    bool            is_synced_{ true }; // Empty local storage is synced with empty buffer.

    void was_desynced() noexcept { is_synced_ = false; }
    void was_synced()   noexcept { is_synced_ = true;  }
};




template<trivially_copyable T>
void UploadBuffer<T>::clear() {
    if (!staged_.empty()) {
        staged_.clear();
        was_desynced();
    }
}


template<trivially_copyable T>
auto UploadBuffer<T>::restage(std::ranges::range auto&& r)
    -> BufferRange
{
    clear();
    std::ranges::copy(r, std::back_inserter(staged_));
    const NumElems num_new_staged = num_staged();
    if (num_new_staged) {
        was_desynced();
    }
    return { OffsetElems{ 0 }, num_new_staged };
}


template<trivially_copyable T>
auto UploadBuffer<T>::stage(std::ranges::range auto&& r)
    -> BufferRange
{
    const OffsetElems old_end{ num_staged() };
    std::ranges::copy(r, std::back_inserter(staged_));
    const OffsetElems new_end{ num_staged() };
    const NumElems num_new_staged{ new_end - old_end };
    if (num_new_staged) {
        was_desynced();
    }
    return { old_end, num_new_staged };
}


template<trivially_copyable T>
void UploadBuffer<T>::ensure_synced() {
    if (!is_synced()) {
        // TODO: Do we care about flags? Because they will be defaulted here.
        expand_to_fit_amortized(buffer_, num_staged());
        buffer_->upload_data(staged_);
        was_synced();
    }
}


template<trivially_copyable T>
auto UploadBuffer<T>::bind_to_ssbo_index(GLuint index)
    -> BindToken<BindingIndexed::ShaderStorageBuffer>
{
    ensure_synced();

    if (num_staged()) {
        // We want to only bind the range that covers staged data,
        // even if amortized allocation might've left the buffer larger.
        const BufferRange range{ OffsetElems{ 0 }, num_staged() };
        return buffer_->template bind_range_to_index<BufferTargetIndexed::ShaderStorage>(range.offset, range.count, index);
    } else /* nothing to bind, unbind the storage */ {
        // TODO: This is scuffed and should not be done like this.
        // The gl layer must be fixed instead to support BindTokens on unbind.
        return RawBuffer<T, GLMutable>::from_id(0).template bind_to_index<BufferTargetIndexed::ShaderStorage>(index);
    }
}


template<trivially_copyable T>
auto UploadBuffer<T>::bind_range_to_ssbo_index(const BufferRange& range, GLuint index)
    -> BindToken<BindingIndexed::ShaderStorageBuffer>
{
    ensure_synced();
    assert(range.offset + range.count <= num_staged());

    if (range.count) {
        return buffer_->template bind_range_to_index<BufferTargetIndexed::ShaderStorage>(range.offset, range.count, index);
    } else {
        return RawBuffer<T, GLMutable>::from_id(0).template bind_to_index<BufferTargetIndexed::ShaderStorage>(index);
    }
}


} // namespace josh
