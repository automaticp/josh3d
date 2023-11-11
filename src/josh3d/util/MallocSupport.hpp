#pragma once
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <type_traits>


namespace josh {


template<typename T>
concept mallocable =
    // See also `std::aligned_alloc`.
    alignof(T) <= alignof(std::max_align_t) &&
    // std::is_implicit_lifetime is maybe more accurate, but it's C++23.
    // Also implicit lifetime is too weak for most use-cases, as the members
    // of the implicit lifetime type would have to be intialized manually,
    // if they are not implicit lifetime types themselves.
    std::is_trivial_v<T> &&
    std::is_standard_layout_v<T>;


struct FreeDeleter {
    void operator()(void* data) const noexcept {
        std::free(data);
    }
};


template<mallocable T>
using unique_malloc_ptr = std::unique_ptr<T, FreeDeleter>;


template<mallocable ArrayT>
    requires (std::is_unbounded_array_v<ArrayT>)
auto malloc_unique(size_t num_elements) noexcept
    -> unique_malloc_ptr<ArrayT>
{
    using T = std::remove_extent_t<ArrayT>;
    return unique_malloc_ptr<ArrayT>{
        (T*)std::malloc(sizeof(T) * num_elements)
    };
}


template<mallocable T>
    requires (!std::is_array_v<T>)
auto malloc_unique() noexcept
    -> unique_malloc_ptr<T>
{
    return unique_malloc_ptr<T>{
        (T*)std::malloc(sizeof(T))
    };
}


} // namespace josh
