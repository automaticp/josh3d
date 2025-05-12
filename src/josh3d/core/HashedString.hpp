#pragma once
#include <cassert>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <entt/core/hashed_string.hpp>


/*
HMM: I think it would be helpful if the integer type of the HashedString was
customizable, so that we could use separate strong ints/enum classes
for declaring string identifiers for different purposes, like:

    enum class ResourceType    : uint32_t {}; // Intentionally empty.
    enum class SceneObjectType : uint32_t {}; // Intentionally empty.

    constexpr auto Camera   = HashedString<SceneObjectType>("Camera"); // No corresponding resource.
    constexpr auto Skeleton = HashedString<ResourceType>("Skeleton");  // No corresponding scene object.

    constexpr auto Mesh    = HashedString<ResourceType>("Mesh");    // Exactly the same underlying values.
    constexpr auto MeshObj = HashedString<SceneObjectType>("Mesh"); // Meaning can value-cast between if needed.

*/
namespace josh {


namespace detail {

template<size_t N, typename CharT>
struct FixedStringStorage {
    using value_type = CharT;
    constexpr auto data() const noexcept -> const value_type* { return _string; }
    value_type _string[N]{};
};

/*
Special case for size 0.
*/
template<typename CharT>
struct FixedStringStorage<0, CharT> {
    using value_type = CharT;
    constexpr auto data() const noexcept -> const value_type* { return nullptr; }
};

} // namespace detail


/*
A reimplementation of entt::hashed_string as a structural type
to support usage in NTTPs. Some code is adopted directly.

Most of the interface replicates that of the entt::hashed_string,
except that the constructors are more strict to ensure this
is not accidentally constructed from a non-literal string.

The class is a fixed-width copy of a string literal with
the same uniqueness semantics as entt::hashed_string.
*/
template<size_t N, typename CharT, typename HashT = uint32_t>
struct BasicFixedHashedString
    : detail::FixedStringStorage<N, CharT>
{
    static constexpr bool is_null = (N == 0);
    using storage_type = detail::FixedStringStorage<N, CharT>;
    using value_type   = CharT;
    using hash_type    = HashT;
    using size_type    = size_t;

    // Constructs a "null" string with hash value of 0.
    //
    // NOTE: This is not the same as constructing from an empty
    // string literal "" - that *won't* result in a hash value of 0.
    consteval BasicFixedHashedString() requires is_null = default;

    // Constructs a hashed string from a string literal.
    //
    // NOTE: The string *must* have the lifetime of the whole program.
    // Prefer to use the literal ""_hs operator to initialize the string.
    consteval BasicFixedHashedString(const value_type (&literal_string)[N]) noexcept;

    using storage_type::data;
    constexpr auto c_str()  const noexcept -> const value_type* { return data();   }
    constexpr auto length() const noexcept -> size_type         { return N - 1;    }
    constexpr auto size()   const noexcept -> size_type         { return length(); }
    constexpr auto hash()   const noexcept -> hash_type         { return _hash;    }
    constexpr auto value()  const noexcept -> hash_type         { return hash();   }

    constexpr operator hash_type()         const noexcept { return hash();  }
    constexpr operator const value_type*() const noexcept { return c_str(); }

    // NOTE: Will rehash the string to the correct value if the hash_type does not match.
    // Beware the interfaces that accept hash_type directly as they implicitly depend on the
    // hashing algorithm and the width of the hash_type. These interfaces should be avoided.
    constexpr operator entt::basic_hashed_string<value_type>() const noexcept {
        if (N) return { data(), length() };
        else   return {};
    }

    constexpr auto operator==(const BasicFixedHashedString& other) const noexcept
        -> bool
    { return hash() == other.hash(); }

    constexpr auto operator<=>(const BasicFixedHashedString& other) const noexcept
        -> std::strong_ordering
    { return operator<=>(hash(), other.hash()); }

    hash_type _hash{};
};


template<size_t N, typename CharT, typename HashT>
consteval BasicFixedHashedString<N, CharT, HashT>::BasicFixedHashedString(
    const value_type (&literal_string)[N]) noexcept
{
    static_assert(N);
    auto strlen [[maybe_unused]] = [](const value_type* str) {
        size_type i{};
        while (*str++ != '\0') ++i;
        return i;
    };
    constexpr auto length = N - 1;
    assert(strlen(literal_string) == length);

    using hash_traits_type = entt::internal::fnv1a_traits<hash_type>;
    constexpr auto offset = hash_traits_type::offset;
    constexpr auto prime  = hash_traits_type::prime;

    _hash = offset;
    for (size_type i{}; i < length; ++i) {
        this->_string[i] = literal_string[i];
        _hash            = (_hash ^ hash_type(literal_string[i])) * prime;
    }
}


/*
An identifier that can be used at compile time in constexpr and NTTP contexts.
In most circumstances, it is best to define a constexpr variable with the string value instead.
*/
template<size_t N>
using FixedHashedString = BasicFixedHashedString<N, char, uint32_t>;
using NullFixedHashedString = FixedHashedString<0>;


/*
Sanity check.
*/
static_assert(FixedHashedString("Hello").value() == entt::hashed_string("Hello").value());


template<BasicFixedHashedString S>
consteval auto operator""_hs() noexcept { return S; }


/*
Identifier for use at runtime in hash tables and such.
Can have a collision of definitions, but should be rare.
*/
using HashedID = uint32_t;


} // namespace josh
