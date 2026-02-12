#pragma once
#include "HashedString.hpp"
#include "Scalars.hpp"
#include "TypeInfo.hpp"
#include <boost/container_hash/hash.hpp>
#include <compare>


namespace josh {

/*
Basic way to identify a system/stage or other "work" unit.
*/
struct SystemKey
{
    TypeIndex type;
    HashedID  instance_id = {}; // This is usually just 0, but could be nonzero to create 2 systems of the same type.
    constexpr auto operator==(const SystemKey&) const noexcept -> bool = default;
    constexpr auto operator<=>(const SystemKey&) const noexcept -> std::strong_ordering = default;
};

inline auto hash_value(const SystemKey& k) noexcept
    -> usize
{
    usize seed = 0;
    boost::hash_combine(seed, k.type);
    boost::hash_combine(seed, k.instance_id);
    return seed;
}

} // namespace josh
