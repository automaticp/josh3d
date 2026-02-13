#pragma once
#include "Scalars.hpp"
#include "Semantics.hpp"
#include <cstddef>
#include <memory_resource>


/*
I don't want the slow, I just want the pools.
*/
namespace josh {


/*
Turns a concrete memory_resource into a (mostly) non-virtual class so that
pointers to it would not generate indirect dispatch calls.
*/
template<typename MemResource>
struct AsMonomorphic final
    : private MemResource
    , private Immovable<AsMonomorphic<MemResource>>
{
    explicit AsMonomorphic(auto&&... args) : MemResource(FORWARD(args)...) {}

    auto allocate(usize bytes, usize alignment = alignof(std::max_align_t))
        -> void*
    {
        return MemResource::do_allocate(bytes, alignment);
    }

    void deallocate(void* ptr, usize bytes, usize alignment = alignof(std::max_align_t)) noexcept
    {
        MemResource::do_deallocate(ptr, bytes, alignment);
    }

    auto operator==(const AsMonomorphic& other) const noexcept
        -> bool
    {
        return MemResource::do_is_equal(static_cast<const std::pmr::memory_resource&>(other));
    }
};


/*
RANT: The "allocator" design is a major crime on humanity with at least 3 infractions:

    - One of them is that allocators are, for some reason, typed;
    - The other is that are assumed to be always copyable;
    - And the third is the entire existence of "pmr" which is the kind of "idea"
    that only makes sense to you if you've been staring at the standard for the
    past 3 weeks and haven't written a single line of code to realise that
    containers are seldom used as vocabulary types, and that the semantics of
    "maybe copy, maybe not, idunno" should have been a warning, not a design
    aspiration.

I tried my best to unscrew this here but I can only do so much.
*/
template<typename MemResource, typename T = void>
struct MonomorphicAllocator {
    using value_type = T;

    template<typename U>
    struct rebind { using other = MonomorphicAllocator<MemResource, U>; };

    template<typename U>
    operator MonomorphicAllocator<MemResource, U>() const noexcept { return { *resource_ptr }; }

    auto allocate(usize n)
        -> T*
    {
        return static_cast<T*>(resource_ptr->allocate(n));
    }

    void deallocate(T* p, usize n) noexcept
    {
        resource_ptr->deallocate(p, n);
    }

    auto operator==(const MonomorphicAllocator& other) const noexcept
        -> bool
    {
        return *resource_ptr == *other.resource_ptr;
    }

    MonomorphicAllocator() = delete;
    MonomorphicAllocator(MemResource* resource_ptr) : resource_ptr(resource_ptr) {}
    MonomorphicAllocator(const MonomorphicAllocator& other) = default;
    MonomorphicAllocator& operator=(const MonomorphicAllocator& other) = default;
    MonomorphicAllocator(MonomorphicAllocator&& other) noexcept = default;
    MonomorphicAllocator& operator=(MonomorphicAllocator&& other) noexcept = default;
    MemResource* resource_ptr;
};


} // namespace josh
