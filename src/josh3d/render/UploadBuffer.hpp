#pragma once
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "CommonConcepts.hpp"
#include "GLAPIBinding.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLBuffers.hpp"
#include "GLMutability.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include <cassert>
#include <iterator>
#include <algorithm>
#include <ranges>


namespace josh {


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
class UploadBuffer
{
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

    // Stage a single element by appending to the existing storage. Effectively push_back().
    auto stage_one(T&& value) -> BufferRange;

    // Obtain a readonly view to the staged storage.
    auto view_staged() const noexcept -> Span<const T> { return staged_; }

    // Commit all data in staged storage to the GPU.
    void ensure_synced();

    // Is the GPU data the same as the staged storage?
    bool is_synced() const noexcept { return is_synced_; }

    // Commit staged data to the GPU and bind the buffer to the `index`.
    auto bind_to_ssbo_index(u32 index)
        -> BindToken<BindingIndexed::ShaderStorageBuffer>;

    // Commit staged data to the GPU and bind the buffer `range` to the `index`.
    auto bind_range_to_ssbo_index(const BufferRange& range, u32 index)
        -> BindToken<BindingIndexed::ShaderStorageBuffer>;

    // Commit staged data to the GPU and bind as the indirect draw buffer.
    auto bind_to_indirect_draw()
        -> BindToken<Binding::DrawIndirectBuffer>;

    // Commit staged data to the GPU and bind as the indirect dispatch buffer.
    auto bind_to_indirect_dispatch()
        -> BindToken<Binding::DispatchIndirectBuffer>;

private:
    Vector<T>       staged_;
    UniqueBuffer<T> buffer_;
    bool            is_synced_ = true; // Empty local storage is synced with empty buffer.

    void was_desynced() noexcept { is_synced_ = false; }
    void was_synced()   noexcept { is_synced_ = true;  }
};


template<trivially_copyable T>
void UploadBuffer<T>::clear()
{
    if (not staged_.empty())
    {
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

    if (num_new_staged)
        was_desynced();

    return { OffsetElems{ 0 }, num_new_staged };
}

template<trivially_copyable T>
auto UploadBuffer<T>::stage(std::ranges::range auto&& r)
    -> BufferRange
{
    const auto old_end = OffsetElems(num_staged());
    std::ranges::copy(r, std::back_inserter(staged_));
    const auto new_end = OffsetElems(num_staged());
    const auto num_new_staged = NumElems(new_end - old_end);

    if (num_new_staged)
        was_desynced();

    return { old_end, num_new_staged };
}

template<trivially_copyable T>
auto UploadBuffer<T>::stage_one(T&& value)
    -> BufferRange
{
    const auto old_end = OffsetElems(num_staged());
    staged_.push_back(FORWARD(value));

    was_desynced();

    return { old_end, 1 };
}

template<trivially_copyable T>
void UploadBuffer<T>::ensure_synced()
{
    if (not is_synced())
    {
        // TODO: Do we care about flags? Because they will be defaulted here.
        const bool was_reallocated =
            expand_to_fit_amortized(buffer_, num_staged());

        if (not was_reallocated)
            buffer_->invalidate_contents();

        buffer_->upload_data(staged_);
        was_synced();
    }
}

template<trivially_copyable T>
auto UploadBuffer<T>::bind_to_ssbo_index(u32 index)
    -> BindToken<BindingIndexed::ShaderStorageBuffer>
{
    ensure_synced();

    if (num_staged())
    {
        // We want to only bind the range that covers staged data,
        // even if amortized allocation might've left the buffer larger.
        const BufferRange range = { OffsetElems{ 0 }, num_staged() };
        return buffer_->template bind_range_to_index<BufferTargetIndexed::ShaderStorage>(range.offset, range.count, index);
    }
    else /* nothing to bind, unbind the storage */
    {
        // TODO: This is scuffed and should not be done like this.
        // The gl layer must be fixed instead to support BindTokens on unbind.
        return RawBuffer<T, GLMutable>::from_id(0).template bind_to_index<BufferTargetIndexed::ShaderStorage>(index);
    }
}

template<trivially_copyable T>
auto UploadBuffer<T>::bind_range_to_ssbo_index(const BufferRange& range, u32 index)
    -> BindToken<BindingIndexed::ShaderStorageBuffer>
{
    ensure_synced();
    assert(range.offset + range.count <= num_staged());

    if (range.count)
    {
        return buffer_->template bind_range_to_index<BufferTargetIndexed::ShaderStorage>(range.offset, range.count, index);
    }
    else
    {
        return RawBuffer<T, GLMutable>::from_id(0).template bind_to_index<BufferTargetIndexed::ShaderStorage>(index);
    }
}

template<trivially_copyable T>
auto UploadBuffer<T>::bind_to_indirect_draw()
    -> BindToken<Binding::DrawIndirectBuffer>
{
    ensure_synced();
    if (num_staged()) return buffer_->template bind<BufferTarget::DrawIndirect>();
    else              return RawBuffer<T>::from_id(0).template bind<BufferTarget::DrawIndirect>();
}

template<trivially_copyable T>
auto UploadBuffer<T>::bind_to_indirect_dispatch()
    -> BindToken<Binding::DispatchIndirectBuffer>
{
    ensure_synced();
    if (num_staged()) return buffer_->template bind<BufferTarget::DispatchIndirect>();
    else              return RawBuffer<T>::from_id(0).template bind<BufferTarget::DispatchIndirect>();
}


} // namespace josh
