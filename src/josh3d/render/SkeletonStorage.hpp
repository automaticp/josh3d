#pragma once
#include "CategoryCasts.hpp"
#include "Common.hpp"
#include "Scalars.hpp"
#include "Skeleton.hpp"
#include <boost/container_hash/hash.hpp>
#include <cassert>


namespace josh {

struct SkeletonID
{
    u64 value;
    constexpr bool operator==(const SkeletonID&) const noexcept = default;
};

inline auto hash_value(SkeletonID id) noexcept
    -> usize
{
    return boost::hash_value(id.value);
}


/*
Quick and dirty "place to put the skeletons into".

This is definetely not the final design.
*/
struct SkeletonStorage
{
    [[nodiscard]]
    auto insert(Skeleton skeleton)
        -> SkeletonID
    {
        assert(skeleton.joints.size() <= Skeleton::max_joints);
        const SkeletonID id = { _storage.size() };
        _storage.push_back(MOVE(skeleton));
        return id;
    }

    auto at(SkeletonID id) const noexcept
        -> const Skeleton&
    {
        assert(id.value < _storage.size());
        return _storage[id.value];
    }

    Vector<Skeleton> _storage;
};

} // namespace josh
