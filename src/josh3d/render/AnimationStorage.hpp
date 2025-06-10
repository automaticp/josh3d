#pragma once
#include "CategoryCasts.hpp"
#include "ContainerUtils.hpp"
#include "Common.hpp"
#include "Scalars.hpp"
#include "SkeletalAnimation.hpp"
#include "SkeletonStorage.hpp"
#include <cassert>


namespace josh {

struct AnimationID
{
    u64 value;
    constexpr bool operator==(const AnimationID&) const noexcept = default;
};

inline auto hash_value(AnimationID id) noexcept
    -> usize
{
    return boost::hash_value(id.value);
}



/*
Quick and dirty "place to put the animations into".

This is definetely not the final design.
*/
struct AnimationStorage
{
    [[nodiscard]]
    auto insert(Animation2lip clip)
        -> AnimationID
    {
        assert(clip.num_joints() <= Skeleton::max_joints);
        const AnimationID id = { _storage.size() };
        _storage.push_back(MOVE(clip));
        auto& anims = _skeleton2anims.try_emplace(clip.skeleton_id).first->second;
        anims.push_back(id);
        return id;
    }

    auto at(AnimationID id) const noexcept
        -> const Animation2lip&
    {
        assert(id.value < _storage.size());
        return _storage[id.value];
    }

    auto anims_for(SkeletonID skeleton_id) const noexcept
        -> Span<const AnimationID>
    {
        if (const auto* anims = try_find_value(_skeleton2anims, skeleton_id))
            return { *anims };
        return {};
    }

    Vector<Animation2lip> _storage;
    HashMap<SkeletonID, SmallVector<AnimationID, 2>> _skeleton2anims;
};

} // namespace josh
