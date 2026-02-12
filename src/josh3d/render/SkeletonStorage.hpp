#pragma once
#include "Common.hpp"
#include "KitchenSink.hpp"
#include "ContainerUtils.hpp"
#include "ID.hpp"
#include "Land.hpp"
#include "Ranges.hpp"
#include "Scalars.hpp"
#include "Skeleton.hpp"
#include <cassert>


namespace josh {

JOSH3D_DERIVE_TYPE(SkeletonID, IDBase<SkeletonID>);

/*
Quick and dirty "place to put the skeletons into".
This is definetely not the final design.

NOTE: Trying to use the `Land` here to support removal.
*/
struct SkeletonStorage
{
    [[nodiscard]]
    auto insert(const Skeleton& skeleton)
        -> SkeletonID
    {
        const usize size = skeleton.joints.size();
        assert(size <= Skeleton::max_joints);

        const SkeletonID id = _new_id();

        const auto range = _land.occupy_amortized(size, { .numer=3, .denom=2 },
            [this](usize new_size) { _resize(new_size); });

        // Since we are accepting the AoS Skeleton representation,
        // we need to split the data out before insertion.
        for (const auto [i, joint] : enumerate(skeleton.joints))
        {
            // Note that we are never "emplacing" elements, only assigning.
            _inv_binds  [range.base + i] = joint.inv_bind;
            _parent_idxs[range.base + i] = joint.parent_idx;
        }

        _table.emplace(id, Entry{
            .name  = skeleton.name,
            .range = range,
        });

        return id;
    }

    struct LookupResult
    {
        StrView          name;
        Span<const mat4> inv_binds;
        Span<const u32>  parent_idxs;
    };

    auto query(SkeletonID id) const noexcept
        -> LookupResult
    {
        if (const Entry* entry = try_find_value(_table, id))
        {
            return {
                .name        = entry->name,
                .inv_binds   = entry->range.subrange_of(_inv_binds),
                .parent_idxs = entry->range.subrange_of(_parent_idxs),
            };
        }
        return {}; // TODO: This should signal failure better eh.
    }

    // This is the part we couldn't have done without the Land.
    auto remove(SkeletonID id)
        -> bool
    {
        if (auto it = _table.find(id);
            it != _table.end())
        {
            const Entry& entry = it->second;

            // NOTE: We do nothing to the data in the vectors here.
            // We only mark that range as "unoccupied" in the Land.

            _land.release(entry.range);
            _table.erase(it);

            return true;
        }
        return false;
    }


    struct Entry
    {
        String      name;
        LandRange<> range;
    };

    u64                        _last_id = 0;
    HashMap<SkeletonID, Entry> _table;
    // This is where we use the Land to store the inv_bind
    // and parent_idx in separate vectors (SoA style).
    Vector<mat4>               _inv_binds;
    Vector<u32>                _parent_idxs;
    Land<>                     _land = {};

    auto _new_id() noexcept -> SkeletonID { return SkeletonID{ _last_id++ }; }
    void _resize(usize new_size)
    {
        // HMM: What's really annoying about the vector is that the elements
        // are always initialized even if they could be left undefined.
        //
        // See also: https://en.cppreference.com/w/cpp/container/vector/resize.html
        // "If value-initialization in overload (1) is undesirable, for example, if
        // the elements are of non-class type and zeroing out is not needed, it can
        // be avoided by providing a custom Allocator::construct."
        // See: https://stackoverflow.com/questions/21028299/is-this-behavior-of-vectorresizesize-type-n-under-c11-and-boost-container/21028912#21028912
        _inv_binds  .resize(new_size);
        _parent_idxs.resize(new_size);
    }
};

} // namespace josh
