#pragma once
#include "Common.hpp"
#include "memory/PageAllocator.hpp"


namespace josh {


/*
A datastructure that manages a stable pool of refcounts.

The refcount itself is not atomic, but can be accessed through
the `atomic_ref()` interface if necessary.

TODO: WIP, not finished.
*/
struct RefCountPool
{
    // TODO: Should we make refcounts "manually" managed too?
    // TODO: Should the refcount type be a customizable "ControlBlock" intead?
    // This will let us stash various flags inside too and may be useful for
    // storing eviction flags and other stuff.


    struct Entry;

    struct VTable
    {
        // Effectively a virtual destructor. Can be null and will not be called in that case.
        void (*on_zero)(Entry* entry, void* data);
    };

    struct Entry
    {
        union
        {
            VTable* vtable;    // Only the first entry in a page.
            usize   refcount;  // All occupied entries except for the first in a page.
            Entry*  next_free; // Next available slot in the free-list.
        };
    };

    struct PageDeleter
    {
        void operator()(void* ptr) const { PageAllocator::deallocate(ptr, 1); }
    };

    using UniquePagePtr = UniquePtr<Entry, PageDeleter>;

    Entry*                _next_free; // TODO: Uhh, how do we initialize this?
    UniquePtr<VTable>     _vtable;    // Allocated separately to keep a stable address. Never null.
    // NOTE: Each page is an array of fixed `page_size / sizeof(Entry)` size.
    Vector<UniquePagePtr> _pages;
};

} // namespace josh
