#pragma once
#include "Common.hpp"
#include "Errors.hpp"
#include "memory/Helpers.hpp"
#include "Ranges.hpp"
#include "Scalars.hpp"
#include "CategoryCasts.hpp"
#include <boost/container_hash/hash.hpp>
#include <iterator>
#include <ranges>


namespace josh {

/*
Land is the datastructure that keeps track of non-overlapping continuous
(address) ranges, and supports reasonably quick lookup, best-fit insertion,
removal and other helpful operations.

The base value type is customizable and does not necessarily have to be pointers.
This allows you to build Lands of index spaces, that are mapped over possibly
address-unstable memory regions (for example, due to amortized growth of the
storage buffers). This also lets you reuse this for mapped files, GPU buffers
and other "fancy" memory.

NOTE: The name is inspired from:
    https://memory-pool-system.readthedocs.io/en/latest/design/land.html
No code has been taken from MPS, however. We're going to poorly NIH it ourselves!

NOTE: This is still WIP and I need to write more usage code.

TODO: We need some "sifting-down" algorithm, either in Land, or in Memory.
*/
template<typename BaseT = uindex, typename SizeT = usize>
struct Land;


template<typename BaseT = uindex, typename SizeT = usize>
struct LandRange
{
    using base_type = BaseT;
    using size_type = SizeT;

    base_type base;
    size_type size;

    // Returns a view that iterates positions of this LandRange.
    constexpr auto view() const noexcept -> std::ranges::view auto;

    // Get the subrange of `r` according to the LandRange, if applicable.
    // This only makes sense if the base_type is an integral index.
    constexpr auto subrange_of(std::ranges::random_access_range auto&& r) const noexcept
        -> std::ranges::random_access_range auto;

    // Returns the "end" position of this LandRange.
    // This may or may not be an iterator, depending on base_type.
    constexpr auto end() const noexcept -> base_type { return base_type(base + size); }

    constexpr explicit operator bool() const noexcept { return bool(size); }
    constexpr auto operator==(const LandRange&) const noexcept -> bool = default;
};

} // namespace josh
template<typename B, typename S>
struct std::hash<josh::LandRange<B, S>>
{
    auto operator()(const josh::LandRange<B, S>& r) const noexcept
        -> std::size_t
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, r.base);
        boost::hash_combine(seed, r.size);
        return seed;
    }
};
namespace josh {


template<typename BaseT, typename SizeT>
struct Land
{
    using base_type  = BaseT;
    using size_type  = SizeT;
    using range_type = LandRange<base_type, size_type>;

    // Construct a Land that covers the `initial_range`. The size of the range
    // designates the `capacity()` of the Land after construction.
    //
    // PRE: `initial_range.size >= 0`.
    Land(range_type initial_range);

    // Construct an empty Land with the base 0.
    Land() : Land(range_type{}) {}

    // Finds the first smallest empty range that fits `size`, occupies and returns it.
    //
    // If no range fits the requested `size`, then an empty range is returned.
    // The user would then likely want to call `expand_by()` with at least `size`
    // to guarantee that the next attempt does not fail.
    //
    // PRE: `size > 0`.
    // POST: `result.size == size` if a fitting range was found, `result.size == 0` otherwise.
    [[nodiscard]] auto try_occupy(size_type size) -> range_type;

    // Finds the first smallest empty range that fits `size`, occupies and returns it.
    //
    // If no range fits the requested `size`, then the amortized growth is applied
    // to the land according to the `ratio` and `size`, and the `on_resize` is invoked
    // with the new size as if by:
    //
    //   `auto new_size = amortized_size_at_least(capacity() + size, capacity(), ratio);`
    //   `this->expand_to(new_size);`
    //   `on_resize(new_size);`
    //
    // The user is expected to adjust to new size inside `on_resize`: resize buffers,
    // extend other dependent ranges, etc.
    //
    // HMM: I think I prefer the "callback" interface over returning a pair of (range, was_resized)
    // because the callback *forces* you to handle the resize event, and even discarding
    // it is done by explicitly passing an empty lambda.
    //
    // PRE: `size > 0`.
    // POST: `result.size == size`.
    template<std::invocable<size_type> F>
    [[nodiscard]] auto occupy_amortized(
        size_type              size,
        GrowthRatio<size_type> ratio,
        F&&                    on_resize)
            -> range_type;

    // Returns a previously occupied `range` back to the land (unoccupies it).
    //
    // PRE: `is_occupied(range)`.
    void release(range_type range);

    // Appends `size` positions to the end of the current land.
    //
    // PRE: `size > 0`.
    void expand_by(size_type size);

    // Expands the current capacity to the new `size`.
    // Does nothing if `capacity() >= size`
    void expand_to(size_type size);

    // Returns true if the `range` is occupied. Try not to lose this information instead.
    auto is_occupied(range_type range) const noexcept -> bool;

    // If there exists an occupied range at `base`, returns it. Otherwise returns an empty range.
    auto range_of(base_type base) const noexcept -> range_type;

    // Returns a view of the occupied ranges ordered ascending by base.
    auto view_occupied() const noexcept -> std::ranges::view auto { return std::views::all(_full_by_base); }

    // Returns a view of the unoccupied ranges ordered ascending by base.
    auto view_empty() const noexcept -> std::ranges::view auto { return std::views::all(_empty_by_base); }

    // Returns a view of the unoccupied ranges ordered ascending by size then base.
    auto view_empty_by_size() const noexcept -> std::ranges::view auto { return std::views::all(_empty_by_size); }

    // Returns the total number of occupied positions, but not necessarily contiguous.
    auto occupied_size() const noexcept -> size_type { return _occupied_size; }

    // Returns the total number of empty positions, but not necessarily contiguous.
    auto total_empty_size() const noexcept -> size_type { return _empty_size; }

    // Returns the size of the largest *contiguous* range that is unoccupied.
    auto largest_empty_size() const noexcept -> size_type;

    // Returns the base of the whole land - the leftmost position
    // that the land occupies.
    //
    // TODO: Currenlty this does not change after initialization
    // and we cannot expand "backwards". This functionality is a bit niche,
    // but there's really no reason for this limitation.
    auto base() const noexcept -> base_type { return _land_range.base; }

    // Returns the total capacity of the land, whether occupied or not.
    // Use `expand*()` functions to modify the capacity.
    auto capacity() const noexcept -> size_type { return _land_range.size; }


    // "Lexicographical" first order by size, then by base.
    // This is needed for first-best-fit lookups.
    struct size_then_base_less_fn
    {
        constexpr auto operator()(const range_type& lhs, const range_type& rhs) const noexcept
            -> bool
        {
            if (lhs.size <  rhs.size) return true;
            if (lhs.size == rhs.size) return (lhs.base < rhs.base);
            return false;
        }
    };

    // Only compares by base.
    // Since the ranges are non-overlapping, this is sufficent.
    struct base_less_fn
    {
        constexpr auto operator()(const range_type& lhs, const range_type& rhs) const noexcept
            -> bool
        {
            return lhs.base < rhs.base;
        }
    };

    range_type _land_range;    // Full land range. This can be partially occupied.
    size_type  _occupied_size; // Total count of all occupied positions.
    size_type  _empty_size;    // Total count of all empty positions.

    OrderedSet<range_type, base_less_fn>           _full_by_base;  // For tracking occupied ranges.
    OrderedSet<range_type, size_then_base_less_fn> _empty_by_size; // For best-fit lookups.
    OrderedSet<range_type, base_less_fn>           _empty_by_base; // For merging adjacent empty ranges.

    // Occupies the range refered to by the iterator `it`, splitting it
    // if the requested `size` is smaller than available.
    auto _occupy(decltype(_empty_by_size)::iterator it, size_type size) -> range_type;

    // Releases the range referred to by `it`, coalescing with adjacent
    // empty ranges, if any.
    void _unoccupy(decltype(_full_by_base)::iterator it);
};


template<typename B, typename S>
constexpr auto LandRange<B, S>::view() const noexcept
    -> std::ranges::view auto
{
    // HMM: Not sure if this works, or works well...
    constexpr bool pointer_like = requires {
        std::ranges::subrange(base, base + size);
    };
    constexpr bool index_like = requires {
        // NOTE: Using views::iota instead of irange since
        // iota does not require evaluating `base + size`
        // which could be not defined for certain types
        // of fancy integers.
        std::views::iota(base, size);
    };
    if constexpr (pointer_like)
    {
        return std::ranges::subrange(base, base + size);
    }
    else if constexpr (index_like)
    {
        return std::views::iota(base, size);
    }
}

template<typename B, typename S>
constexpr auto LandRange<B, S>::subrange_of(
    std::ranges::random_access_range auto&& r) const noexcept
        -> std::ranges::random_access_range auto
{
    const auto beg = std::ranges::begin(r) + base;
    const auto end = beg + size;
    return std::ranges::subrange(beg, end);
}

template<typename B, typename S>
Land<B, S>::Land(range_type initial_range)
    : _land_range   (initial_range)
    , _occupied_size(0)
    , _empty_size   (initial_range.size)
{
    if (initial_range.size)
    {
        _empty_by_size.insert(initial_range);
        _empty_by_base.insert(initial_range);
    }
}

template<typename B, typename S>
auto Land<B, S>::try_occupy(size_type size)
    -> range_type
{
    if (size <= 0)
        panic_fmt("Cannot occupy a range of size {}.", size);

    // Find the best-fit slot by size. The base of 0 in the lookup
    // will guarantee that for an exact match, the first base will
    // be returned - closest to the beginning.
    auto it = _empty_by_size.lower_bound(range_type{ .base={}, .size=size });
    if (it == _empty_by_size.end()) return {};

    return _occupy(it, size);
}

template<typename B, typename S>
auto Land<B, S>::occupy_amortized(
    size_type                        size,
    GrowthRatio<size_type>           ratio,
    std::invocable<size_type> auto&& on_resize)
        -> range_type
{
    auto range = try_occupy(size);

    if (not range)
    {
        const size_type new_size =
            amortized_size_at_least(capacity() + size, capacity(), ratio);

        expand_to(new_size);
        on_resize(new_size);

        range = try_occupy(size);
    }

    assert(range);
    return range;
}

template<typename B, typename S>
void Land<B, S>::release(range_type range)
{
    auto it = _full_by_base.find(range);
    if (it == _full_by_base.end())
        panic_fmt("Released range ({}, {}) is not occupied.", range.base, range.size);

    _unoccupy(it);
}

template<typename B, typename S>
void Land<B, S>::expand_by(size_type size)
{
    if (size <= 0)
        panic_fmt("Invalid size to expand by: {}.", size);

    // The last range would either just be null, or be the last empty range
    // but *only* if that empty range is adjacent to the end (there could
    // be an occupied range at the end instead).
    range_type rightmost_range = { _land_range.end(), 0 };

    auto it = _empty_by_base.end();
    if (it != _empty_by_base.begin())
    {
        it = std::prev(it);
        if (it->end() == _land_range.end())
        {
            rightmost_range = *it;
            _empty_by_size.erase(rightmost_range);
            _empty_by_base.erase(it);
        }
    }

    const range_type new_last_range = {
        .base = rightmost_range.base,
        .size = rightmost_range.size + size
    };

    _empty_by_base.insert(_empty_by_base.end(), new_last_range);
    _empty_by_size.insert(new_last_range);

    _empty_size      += size;
    _land_range.size += size;
}

template<typename B, typename S>
void Land<B, S>::expand_to(size_type size)
{
    if (_land_range.size >= size)
        return;

    expand_by(size - _land_range.size);
}

template<typename B, typename S>
auto Land<B, S>::is_occupied(range_type range) const noexcept
    -> bool
{
    return _full_by_base.contains(range);
}

template<typename B, typename S>
auto Land<B, S>::range_of(base_type base) const noexcept
    -> range_type
{
    // Due to the comparator only comparing bases, any two ranges
    // with the same base will compare "equivalent".
    auto it = _full_by_base.find(range_type{ base, 0 });
    if (it != _full_by_base.end()) return *it;
    return {};
}

template<typename B, typename S>
auto Land<B, S>::largest_empty_size() const noexcept
    -> size_type
{
    auto it = _empty_by_size.rend();
    if (it != _empty_by_size.rbegin()) return it->size;
    return {};
}

template<typename B, typename S>
auto Land<B, S>::_occupy(decltype(_empty_by_size)::iterator it, size_type size)
    -> range_type
{
    assert(it != _empty_by_size.end());
    assert(size > 0);
    assert(it->size >= size);

    // NOTE: range_type must be copyable.
    const range_type src_range = *it;

    // Using extract()/insert() lets us avoid memory allocation for the node
    // in the destination set. We effectively "splice" a node between sets.
    //
    // Now, being real here, the memory allocator is not dumb an would have
    // given you back exactly the same memory freed in initial clear().
    //
    // Being *extra* real, the overhead of these operations is dwarfed by
    // repeated rebalancing of the trees themselves.
    auto node = _empty_by_size.extract(it);
    _empty_by_base.erase(src_range);

    // Split the range into left and right if the available size exceeds requested.
    const range_type left_range  = { src_range.base,   size                  };
    const range_type right_range = { left_range.end(), src_range.size - size };

    node.value() = left_range;
    _full_by_base.insert(MOVE(node));

    if (right_range)
    {
        _empty_by_size.insert(right_range);
        _empty_by_base.insert(right_range);
    }

    _empty_size    -= size;
    _occupied_size += size;

    return left_range;
}

template<typename B, typename S>
void Land<B, S>::_unoccupy(decltype(_full_by_base)::iterator it)
{
    assert(it != _full_by_base.end());
    const range_type range = *it;

    // NOTE: Using extract() here even though we later will likely
    // coalesce adjacent empty ranges, so the full "returned" range might
    // be wider than `range`. That is okay, because we can just overwrite
    // the value stored in the node.
    auto node = _full_by_base.extract(it);

    // We need to find closest left and right empty ranges, if any,
    // and "merge" with them. Coalescing is done eagerly on every
    // remove() as that gives us the strongest guarantee: at no point
    // there could ever be 2 or more adjacent empty ranges.

    // This is a combined lower_bound() + upper_bound() call.
    // This only compares the `base` value of the range.
    // lower_bound() -> `lb` - first "not less-then" value,
    // upper_bound() -> `ub` - first "greater-than" value.
    auto [lb, ub] = _empty_by_base.equal_range(range);

    // NOTE: Given that overlapping ranges are forbidden, each element
    // in the `_empty_by_base` is unique by `base` (it's a set), and
    // the pair `lb`, `ub` cannot possibly "span" more than 2 elements.
    //
    // Additionally, because the `range` cannot ever be in `_empty_by_range`
    // before this operation, both `lb` and `ub` must refer to *the same*
    // element that follows "the gap", or refer to none at all.
    assert(lb == ub);

    // NOTE: Checks *only* if the right edge of lhs touches
    // the left edge of rhs, not the other way around.
    const auto are_adjacent = [](const range_type& lhs, const range_type& rhs)
    {
        return (lhs.end() == rhs.base);
    };

    // We consider each side separately for coalescing.
    auto left  = _empty_by_base.end();
    auto right = _empty_by_base.end();
    bool merge_left  = false;
    bool merge_right = false;

    if (lb != _empty_by_base.end() and
        lb != _empty_by_base.begin())
    {
        left = std::prev(lb);
        if (are_adjacent(*left, range))
            merge_left = true;
    }

    if (ub != _empty_by_base.end())
    {
        right = ub;
        if (are_adjacent(range, *right))
            merge_right = true;
    }

    const base_type base =
        merge_left ? left->base : range.base;

    const size_type size =
        (merge_left  ? left->size  : 0) +
        (range.size) +
        (merge_right ? right->size : 0);

    const range_type merged_range = { base, size };

    // Then we remove the previous ranges where applicable.
    //
    // NOTE: Here it is critical that iterators are not invalidated
    // on `erase()` for whichever container is used to represet a set.
    if (merge_left)
    {
        _empty_by_base.erase(left);
        _empty_by_size.erase(*left);
    }

    if (merge_right)
    {
        _empty_by_base.erase(right);
        _empty_by_size.erase(*right);
    }

    // Overwriting the value in the node is legal, which is quite
    // thoughtful of the standard library. Unfortunately, there's
    // only *one* node. And we need two...
    node.value() = merged_range;
    _empty_by_base.insert(MOVE(node));
    _empty_by_size.insert(merged_range);

    _empty_size    += range.size;
    _occupied_size -= range.size;
}





} // namespace josh
