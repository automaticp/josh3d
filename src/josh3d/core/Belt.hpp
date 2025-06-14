#pragma once
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "ContainerUtils.hpp"
#include "TypeInfo.hpp"


namespace josh {


/*
Communication and data exchange channel for some pipeline.

One stage puts things on the belt, the following stages
observe, modify, take, drop it, etc. Each *thing* is identified
by its type slot T.

Constness is not enforced. Too much to bother with.
Ordering is not enforced. (Could be?)

The name is... something about conveyors or whatever.
*/
struct Belt
{
    // Insert or assign an item to the slot T.
    // Additional lives can be added to the item to make it "survive" N sweeps.
    // Use this to pass data across frames.
    template<typename T>
    auto put(T&& item, u32 extra_lives = 0) -> T&;

    // Insert or assign a non-owning reference to an item to the slot T.
    // Additional lives can be added to the item to make it "survive" N sweeps.
    // Use this to pass data across frames.
    template<typename T>
    auto put_ref(T& item_ref, u32 extra_lives = 0) -> T&;

    // Get a reference to the item in slot T. Will panic if the slot is empty.
    template<typename T>
    auto get() -> T&;

    // Get a pointer to the item in slot T or nullptr if the slot is empty.
    template<typename T>
    auto try_get() -> T*;

    // Check if the slot T contains an item.
    template<typename T>
    auto has() const noexcept -> bool;

    // Remove an item from slot T if it exists.
    // Returns true if an item was indeed removed.
    template<typename T>
    auto drop() -> bool;

    // Decrement lives of all items and remove them if the life count
    // reaches 0. Returns the number of items removed.
    auto sweep() -> usize;

    // Returns the total number of items in all slots.
    auto size() const noexcept -> usize;

    struct Package
    {
        u32     lives;
        bool    is_reference;
        Any<40> item;
    };

    HashMap<TypeIndex, Package> _packages;
};


template<typename T>
auto Belt::put(T&& item, u32 extra_lives)
    -> T&
{
    const u32 lives = extra_lives + 1;
    auto [it, was_emplaced] = _packages.try_emplace(type_id<T>(), lives, false, FORWARD(item));
    if (not was_emplaced)
    {
        // Then assign instead.
        it->second = Package{ lives, false, FORWARD(item) };
    }
    return boost::any_cast<T&>(it->second.item);
}

template<typename T>
auto Belt::put_ref(T& item_ref, u32 extra_lives)
    -> T&
{
    const u32 lives = extra_lives + 1;
    auto [it, was_emplaced] = _packages.try_emplace(type_id<T>(), lives, true, &item_ref);
    if (not was_emplaced)
    {
        it->second = Package{ lives, true, &item_ref };
    }
    return *boost::any_cast<T*>(it->second.item);
}

template<typename T>
auto Belt::get()
    -> T&
{
    T* item_ptr = try_get<T>();
    if (not item_ptr)
        panic_fmt("Attempted to get an item with T={} but it is not on the belt.",
            type_id<T>().pretty_name());
    return *item_ptr;
}

template<typename T>
auto Belt::try_get()
    -> T*
{
    const TypeIndex type = type_id<T>();
    Package* package = try_find_value(_packages, type);
    if (not package)
        return nullptr;

    if (package->is_reference)
    {
        // "References" are stored as a pointer.
        assert(package->item.type() == type_id<T*>());
        return any_cast<T*>(package->item);
    }
    else
    {
        // Panic when it is user's bug, assert when it is ours.
        assert(package->item.type() == type_id<T>());
        return &any_cast<T&>(package->item);
    }
}

template<typename T>
auto Belt::has() const noexcept
    -> bool
{
    return _packages.contains(type_id<T>());
}

template<typename T>
auto Belt::drop()
    -> bool
{
    return _packages.erase(type_id<T>());
}

inline auto Belt::sweep()
    -> usize
{
    usize num_erased = 0;
    auto it = _packages.begin();
    while (it != _packages.end())
    {
        Package& p = it->second;
        // NOTE: The life count could be 0 if the user
        // passed in -1 to `extra_lives`, in which case
        // the item will persist until overwritten or
        // explicitly dropped. I'm not sure if this should
        // be advertized as part of the interface though.
        if (p.lives)
        {
            if (--p.lives == 0)
            {
                it = _packages.erase(it);
                ++num_erased;
                continue;
            }
        }
        ++it;
    }
    return num_erased;
}

inline auto Belt::size() const noexcept
    -> usize
{
    return _packages.size();
}

} // namespace josh
