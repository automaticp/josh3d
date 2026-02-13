#include "PageAllocator.hpp"
#include "BuildConfig.hpp"
#include "Errors.hpp"
#include "KitchenSink.hpp"
#include <cassert>
#include <new>
#ifdef JOSH3D_OS_LINUX
#include <sys/mman.h>
#include <unistd.h>
#else
#error "TODO: Platform support."
#endif

namespace josh {

#ifdef JOSH3D_OS_LINUX

const usize PageAllocator::page_size = sysconf(_SC_PAGE_SIZE);

auto PageAllocator::allocate(usize nbytes) -> void*
{
    assert(nbytes > 0);
    const usize length = next_multiple_of(page_size, nbytes);
    void* ptr = mmap(nullptr, length, (PROT_READ | PROT_WRITE), MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) // NOTE: This is bizzare. Can you map the zero page? On embedded maybe?
    {
        if (errno == ENOMEM)
            throw std::bad_alloc();
        else
            panic_fmt("Invalid mmap(0, {}, ...) call.", length);
    }
    return ptr;
}

void PageAllocator::deallocate(void* ptr, usize nbytes)
{
    const usize length = next_multiple_of(page_size, nbytes);
    int rc = munmap(ptr, length);
    if (rc != 0)
        panic_fmt("Invalid munmap({}, {}) call.", ptr, length);
}

#endif

} // namespace josh
