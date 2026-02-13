#pragma once
#include "Scalars.hpp"


namespace josh {

/*
An allocator that allocates whole pages, aligned on the page boundary.
Just wraps `mmap()` on linux.

FIXME: This is not perfect, I quickly threw this together due to an immediate need.
*/
struct PageAllocator
{
    // Any allocation under `page_size` will just give you a single page.
    // Allocations above that will round up to the multiple of pages.
    // This is to be conformant with the "allocator interface".
    // Just call `allocate(1)` if you need a single page.
    //
    // PRE: nbytes > 0
    [[nodiscard]]
    static auto allocate(usize nbytes) -> void*;

    static void deallocate(void* ptr, usize nbytes);

    static const usize page_size;
};

} // namespace josh
