#pragma once
#include "Scalars.hpp"
#include <boost/container_hash/hash.hpp>
#include <concepts>


namespace josh {

/*
IDs are used as a primary way to refer to runtime-specific resources.
The exact meaning of the value of each concrete ID type depends
on the storage/pool that issued the ID. It could be an index,
a table key, an address or whatever else you can stuff into 64 bits.
*/
template<typename CRTP>
struct IDBase
{
    u64 _value;

    explicit constexpr IDBase(u64 value) noexcept : _value(value) {}

    friend constexpr auto operator==(const CRTP& lhs, const CRTP& rhs) noexcept
        -> bool
    {
        return lhs._value == rhs._value;
    }
};

template<typename T>
auto hash_value(IDBase<T> id) noexcept
    -> usize
{
    return boost::hash_value(id._value);
}


namespace detail {
struct NullID
{
    template<typename T> requires std::derived_from<T, IDBase<T>>
    constexpr operator T() const noexcept { return T{ u64(-1) }; }
};
} // namespace detail

/*
Special ID-convertible value that produces a "null" ID of any type.
The underlying value is chosen to be -1 as 0 has more common use cases,
such as when being used as an array index.
*/
constexpr auto nullid = detail::NullID();


/*
Type-erased ID type.
*/
struct AnyID : IDBase<AnyID>
{
    template<typename CRTP>
    AnyID(IDBase<CRTP> base) : IDBase<AnyID>(base._value) {}

    template<typename T> requires std::derived_from<T, IDBase<T>>
    constexpr auto cast_back() const noexcept -> T { return T{ _value }; }
};


} // namespace josh
