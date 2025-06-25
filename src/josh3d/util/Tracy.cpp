#ifdef TRACY_ENABLE
#include "BuildConfig.hpp"
#include <tracy/Tracy.hpp>
#include <cstddef>
#include <cstdlib>
#include <new>


/*
Instrumented global allocation operators.
*/

auto operator new(std::size_t size)
    -> void*
{
    if (void* ptr = std::malloc(size))
    {
        TracySecureAlloc(ptr, size);
        return ptr;
    }
    throw std::bad_alloc{};
}

auto operator new(std::size_t size, std::align_val_t alignment)
    -> void*
{
    // NOTE: aligned_alloc is not supported on MSVC. Use _aligned_malloc, _aligned_free.
#ifdef JOSH3D_OS_WINDOWS
    // Why is the order of arguments reversed?
    if (void* ptr = _aligned_malloc(size, std::size_t(alignment)))
#else
    if (void* ptr = std::aligned_alloc(std::size_t(alignment), size))
#endif
    {
        TracySecureAlloc(ptr, size);
        return ptr;
    }
    throw std::bad_alloc{};
}

void operator delete(void* ptr) noexcept
{
    TracySecureFree(ptr);
    std::free(ptr);
}

void operator delete(void* ptr, std::align_val_t) noexcept
{
    TracySecureFree(ptr);
#ifdef JOSH3D_OS_WINDOWS
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}


#endif // TRACY_ENABLE
