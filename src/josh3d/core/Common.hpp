#pragma once
#include <boost/any/basic_any.hpp>
#include <boost/any/unique_any.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <functional>
#include <map>
#include <memory>
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


// TODO: Enable transparent lookup?
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
    typename ValueT,
    typename CompareF = std::less<KeyT>,
    typename AllocatorT = std::allocator<std::pair<const KeyT, ValueT>>
>
using OrderedMap = std::map<KeyT, ValueT, CompareF, AllocatorT>;


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
    size_t   NumElemsInlineV,
    typename AllocatorT = void,
    typename OptionsT = void
>
using SmallVector = boost::container::small_vector<T, NumElemsInlineV, AllocatorT, OptionsT>;


// NOTE: Alignment is suggestive. If alignment of a type is stricter, it will be lifted to heap.
template<
    size_t InlineSizeV = 3 * sizeof(void*),
    size_t InlineAlignV = alignof(void*)
>
using Any = boost::anys::basic_any<InlineSizeV, InlineAlignV>;


// NOTE: This always heap-allocates. This is to ensure that immovable types can be stored too.
// Potentially, we might need some MoveOnlyAny for SBO optimized move-only types.
using UniqueAny = boost::anys::unique_any;


template<typename T>
using Optional = std::optional<T>;
using std::nullopt;


} // namespace josh
