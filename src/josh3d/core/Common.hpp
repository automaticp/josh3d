#pragma once
#include <boost/any/basic_any.hpp>
#include <boost/any/unique_any.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <vector>


/*
Common public vocabulary and core utilities.
*/
namespace josh {


/*
I have a 2-hour long lecture about why std::byte was a mistake.
*/
using byte = unsigned char;


// TODO: as_bytes(span);


template<
    typename KeyT,
    typename ValueT,
    typename HashF = boost::hash<KeyT>,
    typename KeyEqualF = std::equal_to<KeyT>,
    typename AllocatorT = std::allocator<std::pair<const KeyT, ValueT>>
>
using HashMap = boost::unordered::unordered_flat_map<KeyT, ValueT, HashF, KeyEqualF, AllocatorT>;


template<
    typename KeyT,
    typename HashF = boost::hash<KeyT>,
    typename KeyEqualF = std::equal_to<KeyT>,
    typename AllocatorT = std::allocator<KeyT>
>
using HashSet = boost::unordered::unordered_flat_set<KeyT, HashF, KeyEqualF, AllocatorT>;


template<
    typename KeyT,
    typename ValueT,
    typename CompareF = std::less<KeyT>,
    typename AllocatorT = std::allocator<std::pair<const KeyT, ValueT>>
>
using OrderedMap = std::map<KeyT, ValueT, CompareF, AllocatorT>;


template<
    typename KeyT,
    typename CompareF = std::less<KeyT>,
    typename AllocatorT = std::allocator<KeyT>
>
using OrderedSet = std::set<KeyT, CompareF, AllocatorT>;


template<
    typename T,
    typename AllocatorT = std::allocator<T>
>
using Vector = std::vector<T, AllocatorT>;


template<
    typename T,
    size_t   CapacityV,
    typename OptionsT = void
>
using StaticVector = boost::container::static_vector<T, CapacityV, OptionsT>;


template<
    typename T,
    size_t   SBOElemCountV,
    typename AllocatorT = void,
    typename OptionsT = void
>
using SmallVector = boost::container::small_vector<T, SBOElemCountV, AllocatorT, OptionsT>;


// NOTE: Alignment is suggestive. If alignment of a type is stricter, it will be lifted to heap.
template<
    size_t SBOSizeV = 3 * sizeof(void*),
    size_t SBOAlignV = alignof(void*)
>
using Any = boost::anys::basic_any<SBOSizeV, SBOAlignV>;


// NOTE: This always heap-allocates. This is to ensure that immovable types can be stored too.
// Potentially, we might need some MoveOnlyAny for SBO optimized move-only types.
using UniqueAny = boost::anys::unique_any;


template<typename T>
using Optional = std::optional<T>;
using std::nullopt;


template<typename T>
using Span = std::span<T>;

template<typename T>
auto as_bytes(const Span<T>& span) noexcept
    -> Span<byte>
{ return { (byte*)span.data(), span.size_bytes() }; }

template<typename T> requires std::is_const_v<T>
auto as_bytes(const Span<T>& span) noexcept
    -> Span<const byte>
{ return { (const byte*)span.data(), span.size_bytes() }; }

template<std::ranges::contiguous_range R>
auto to_span(R&& r) noexcept
    -> Span<std::ranges::range_value_t<R>>
{
    return { r.data(), r.size() };
}

template<typename DstT, typename SrcT>
auto pun_span(Span<SrcT> src) noexcept
    -> Span<DstT>
{
    static_assert(alignof(DstT) >= alignof(SrcT));
    assert((src.size_bytes()      % sizeof(DstT))  == 0);
    assert((uintptr_t(src.data()) % alignof(DstT)) == 0);
    return { std::launder(reinterpret_cast<DstT*>(src.data())), src.size_bytes() / sizeof(DstT) };
}


// TODO: That horrible operator==() is giving me nightmares.
using StrView = std::string_view;
using namespace std::string_view_literals;


using String = std::string;


} // namespace josh
