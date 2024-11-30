#pragma once
#include "CommonConcepts.hpp"
#include "GLAPICore.hpp"
#include "GLBuffers.hpp"
#include "GLFenceSync.hpp"
#include "GLObjectHelpers.hpp"
#include "GLObjects.hpp"
#include <algorithm>


namespace josh {


/*
Buffer wrapper for a particular usage pattern:

    1. Data needs to be retrieved from the GPU periodically, likely every frame;
    2. Data is not very large, and the amount of ReadbackBuffers in-flight is also not;
    3. It is acceptable that the data comes back late by a couple of frames
       (for a heuristic, approximation, or a smoothly changing parameter)

Should likely be used with a small ring buffer or a queue to account for latency.

TODO: The interface does not allow you to reuse the same storage
for multiple readbacks, but aside from the fact that you can't
reuse it before the result becomes available (latency), I don't
see a reason why you wouldn't be able to do that.

If the readback copy has been already realized and the change is
visible client side, then there's no more dependencies that can
be in-flight, and reuse is perfectly fine.

Obviously don't reuse while the result is not yet available.

NOTE: There's a significant overhead for insertion and querying
the FenceSync outlined below. Although it is not a GPU stall,
but rather a CPU blocking on a forced flush. Not sure what to
do about it right now.
*/
template<trivially_copyable T>
class ReadbackBuffer {
public:
    // Create a ReadbackBuffer from the data contents of the `other` buffer.
    static auto fetch(RawBuffer<T, GLConst> other)
        -> ReadbackBuffer<T>;

    // Check if the data is available for a non-blocking read.
    bool is_available() const noexcept;

    // Number of times `is_available()` was called before it returned true.
    // Can be used as a rough measure of latency in number of frames.
    auto times_queried_until_available() const noexcept -> size_t;

    // Size of the readbuck buffer.
    auto num_elements() const noexcept -> size_t;

    // Read the buffer contents into the client-side memory.
    // Size of the `out_buf` should be large enough to fit `num_elements()`.
    //
    // If `is_available()` is false during this call, the read will block.
    void get_data_into(std::span<T> out_buf) const noexcept;

private:
    ReadbackBuffer(
        UniqueBuffer<T> buffer,
        UniqueFenceSync fence
    )
        : buffer_{ std::move(buffer) }
        , fence_ { std::move(fence)  }
    {}

    UniqueBuffer<T>    buffer_;
    UniqueFenceSync    fence_;
    mutable size_t     num_queries_{ 0 };

};


template<trivially_copyable T>
auto ReadbackBuffer<T>::fetch(RawBuffer<T, GLConst> other)
    -> ReadbackBuffer<T>
{
    const NumElems num_elements = other.get_num_elements();
    const StoragePolicies policies{
        .mode        = StorageMode::StaticServer,
        .mapping     = PermittedMapping::Read,
        .persistence = PermittedPersistence::Persistent,
    };

    UniqueBuffer<T> readback = allocate_buffer<T>(num_elements, policies);

    // SynchronizeOnMap is needed to make sure the storage is actually allocated.
    // We will drop the mapped pointer here, and retrieve it later when reading the value.
    const MappingReadPolicies mapping_policies{
        .pending_ops = PendingOperations::SynchronizeOnMap,
        .persistence = Persistence::Persistent,
    };
    auto mapped [[maybe_unused]] = readback->map_for_read(mapping_policies);

    other.copy_data_to(readback.get(), num_elements);

    // The barrier makes the update to the mapped region visible,
    // but it is not issued immediately, instead it goes into the
    // command queue to be executed later, after the copy operation.
    glapi::memory_barrier(BarrierMask::ClientMappedBufferBit);

    // To "query" when the memory is actually available for reading,
    // we insert the fence after the barrier.
    //
    // NOTE: Creation of this fence seems to be quite expensive
    // as per profiling, as this forces a flush for the context.
    UniqueFenceSync fence;

    return { std::move(readback), std::move(fence) };
}


template<trivially_copyable T>
bool ReadbackBuffer<T>::is_available() const noexcept {
    // NOTE: This has_signaled() call (which wraps glGetSynciv()),
    // is also particularly expensive and seems to translate to
    // __client_wait_sync() in the driver, which in turn triggers
    // an equivalent of a "flush" and submits some draw calls.
    //
    // The result turns out to be equivalent to:
    //   if (fence_->flush_and_wait_for(0) != TimeoutExpired) { ...
    //
    if (fence_->has_signaled()) {
        return true;
    } else {
        ++num_queries_;
        return false;
    }
}


template<trivially_copyable T>
auto ReadbackBuffer<T>::times_queried_until_available() const noexcept
    -> size_t
{
    return num_queries_;
}


template<trivially_copyable T>
auto ReadbackBuffer<T>::num_elements() const noexcept
    -> size_t
{
    return buffer_->get_num_elements();
}


template<trivially_copyable T>
void ReadbackBuffer<T>::get_data_into(std::span<T> out_buf) const noexcept {
    assert(out_buf.size() >= buffer_->get_num_elements());

    const RawFenceSync<>::nanoseconds timeout{ 1'000'000 };
    while (fence_->flush_and_wait_for(timeout) == SyncWaitResult::TimeoutExpired) {
        // Spin if not available.
    }

    auto mapped = buffer_->get_current_mapping_span_for_read();
    std::ranges::copy(mapped, out_buf.begin());
}




} // namespace josh
