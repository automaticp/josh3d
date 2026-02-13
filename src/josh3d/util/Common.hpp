#pragma once
#include "Scalars.hpp"
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <vector>


/*
Common public vocabulary and core utilities. PCH save us all.
*/
namespace josh {


/* Containers. */

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
    typename AllocatorT,
    typename HashF = boost::hash<KeyT>,
    typename KeyEqualF = std::equal_to<KeyT>
>
using HashMapA = boost::unordered::unordered_flat_map<KeyT, ValueT, HashF, KeyEqualF, AllocatorT>;

template<
    typename KeyT,
    typename HashF = boost::hash<KeyT>,
    typename KeyEqualF = std::equal_to<KeyT>,
    typename AllocatorT = std::allocator<KeyT>
>
using HashSet = boost::unordered::unordered_flat_set<KeyT, HashF, KeyEqualF, AllocatorT>;

template<
    typename KeyT,
    typename AllocatorT,
    typename HashF = boost::hash<KeyT>,
    typename KeyEqualF = std::equal_to<KeyT>
>
using HashSetA = boost::unordered::unordered_flat_set<KeyT, HashF, KeyEqualF, AllocatorT>;

template<
    typename KeyT,
    typename ValueT,
    typename CompareF = std::less<KeyT>,
    typename AllocatorT = std::allocator<std::pair<const KeyT, ValueT>>
>
using OrderedMap = std::map<KeyT, ValueT, CompareF, AllocatorT>;

template<
    typename KeyT,
    typename ValueT,
    typename AllocatorT,
    typename CompareF = std::less<KeyT>
>
using OrderedMapA = std::map<KeyT, ValueT, CompareF, AllocatorT>;

template<
    typename KeyT,
    typename CompareF = std::less<KeyT>,
    typename AllocatorT = std::allocator<KeyT>
>
using OrderedSet = std::set<KeyT, CompareF, AllocatorT>;

template<
    typename KeyT,
    typename AllocatorT,
    typename CompareF = std::less<KeyT>
>
using OrderedSetA = std::set<KeyT, CompareF, AllocatorT>;

template<
    typename T,
    typename AllocatorT = std::allocator<T>
>
using Vector = std::vector<T, AllocatorT>;

template<
    typename T,
    usize    CapacityV,
    typename OptionsT = void
>
using StaticVector = boost::container::static_vector<T, CapacityV, OptionsT>;

template<
    typename T,
    usize    SBOElemCountV,
    typename AllocatorT = void,
    typename OptionsT = void
>
using SmallVector = boost::container::small_vector<T, SBOElemCountV, AllocatorT, OptionsT>;

template<
    typename T,
    usize    N
>
using Array = std::array<T, N>;


/* Values. */

using String = std::string;

template<typename AllocatorT>
using StringA = std::basic_string<char, std::char_traits<char>, AllocatorT>;

using Path = std::filesystem::path;

template<typename T>
using Optional = std::optional<T>;
constexpr auto nullopt = std::nullopt;

template<typename ...Ts>
using Variant = std::variant<Ts...>;


/* Views. */

template<typename T, usize Extent = std::dynamic_extent>
using Span = std::span<T, Extent>;

/*
Completely broken CTAD for typedefs of std::span is not helping anyone.

NOTE: I hope this is a bug in the GCC implementation.
TODO: I think this might be because I did not forward the Extent parameter.
I do now, maybe it works?
*/
constexpr auto make_span(auto&&... args)
{
    return std::span(static_cast<decltype(args)&&>(args)...);
}

template<typename T>
auto as_bytes(const Span<T>& span) noexcept -> Span<byte>
{
    return { (byte*)span.data(), span.size_bytes() };
}

template<typename T> requires std::is_const_v<T>
auto as_bytes(const Span<T>& span) noexcept -> Span<const byte>
{
    return { (const byte*)span.data(), span.size_bytes() };
}

template<std::ranges::contiguous_range R>
auto to_span(R&& r) noexcept
{
    return std::span{ r.data(), r.size() };
}

template<typename DstT, typename SrcT>
auto pun_span(Span<SrcT> src) noexcept -> Span<DstT>
{
    static_assert(alignof(DstT) >= alignof(SrcT));
    assert((src.size_bytes()    % sizeof(DstT))  == 0);
    assert((uintptr(src.data()) % alignof(DstT)) == 0);
    return { std::launder(reinterpret_cast<DstT*>(src.data())), src.size_bytes() / sizeof(DstT) };
}

// TODO: That horrible operator==() is giving me nightmares.
using StrView = std::string_view;
using namespace std::string_view_literals;


/* Owners. */

template<typename T, typename DeleterT = std::default_delete<T>>
using UniquePtr = std::unique_ptr<T, DeleterT>;
using std::make_unique;

template<typename T>
using SharedPtr = std::shared_ptr<T>;
using std::make_shared;


} // namespace josh
