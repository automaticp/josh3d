#pragma once
#include "AnyRef.hpp"
#include "AsyncCradle.hpp"
#include "CommonConcepts.hpp"
#include "Resource.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceLoader.hpp"
#include "ResourceRegistry.hpp"
#include "RuntimeError.hpp"
#include "TaskCounterGuard.hpp"
#include "TypeInfo.hpp"
#include <boost/container_hash/hash.hpp>
#include <fmt/core.h>
#include <utility>


namespace josh {


namespace detail {
struct UnpackerKey {
    ResourceType resource_type;
    TypeIndex    destination_type;
    auto operator==(const UnpackerKey&) const noexcept -> bool = default;
};
inline auto hash_value(const UnpackerKey& u) noexcept
    -> size_t
{
    size_t seed = 0;
    boost::hash_combine(seed, u.resource_type);
    boost::hash_combine(seed, u.destination_type);
    return seed;
}
} // namespace detail


class ResourceUnpackerContext;


/*
Unpacking is the process of converting the intermediate resource
representation into its final "consumable" form for the target
destination.

The destination of unpacking could be any system that needs to
work on resulting data, for example the scene's mesh and material
components, skeleton/animation storage of the animation system, etc.

Unpacking never loads data from disk directly, and instead retrieves
all the data through the ResourceRegistry, which is responsible for
loading, caching and evicting actual resource data.
*/
class ResourceUnpacker {
public:
    ResourceUnpacker(
        ResourceDatabase& resource_database,
        ResourceRegistry& resource_registry,
        ResourceLoader&   resource_loader,
        AsyncCradleRef    async_cradle
    )
        : resource_database_(resource_database)
        , resource_registry_(resource_registry)
        , resource_loader_  (resource_loader)
        , cradle_           (async_cradle)
    {}

    template<ResourceType TypeV, typename DestinationT,
        of_signature<Job<>(ResourceUnpackerContext, UUID, DestinationT)> UnpackerF>
    void register_unpacker(UnpackerF&& f);

    template<ResourceType TypeV, typename DestinationT>
    auto unpack(UUID uuid, DestinationT destination)
        -> Job<>;

    template<typename DestinationT>
    auto unpack_any(UUID uuid, DestinationT destination)
        -> Job<>;

private:
    friend ResourceUnpackerContext;
    ResourceDatabase&  resource_database_;
    ResourceRegistry&  resource_registry_;
    ResourceLoader&    resource_loader_;
    AsyncCradleRef     cradle_;

    using key_type      = detail::UnpackerKey;
    using unpacker_func = UniqueFunction<Job<>(ResourceUnpackerContext, UUID, AnyRef)>;
    using dtable_type   = HashMap<key_type, unpacker_func>;

    dtable_type dispatch_table_;

    auto _unpack(const key_type& key, UUID uuid, AnyRef destination)
        -> Job<>;
};


class ResourceUnpackerContext {
public:
    auto& resource_registry()  noexcept { return self_.resource_registry_;         }
    auto& resource_loader()    noexcept { return self_.resource_loader_;           }
    auto& thread_pool()        noexcept { return self_.cradle_.loading_pool;       }
    auto& offscreen_context()  noexcept { return self_.cradle_.offscreen_context;  }
    auto& completion_context() noexcept { return self_.cradle_.completion_context; }
    auto& task_counter()       noexcept { return self_.cradle_.task_counter;       }
    auto& local_context()      noexcept { return self_.cradle_.local_context;      }
    // TODO: Should we instead expose only the relevant unpack_*_to() functions?
    auto& unpacker()           noexcept { return self_;                            }

    // FIXME: This should not exists, but it has to for now because some unpackers
    // spawn child unpacking tasks directly instead of going through the interface.
    // This is because they do not have respective unpackers defined for their subtasks
    // and would simply fail otherwise.
    [[nodiscard]]
    auto child_context() const noexcept
        -> ResourceUnpackerContext
    {
        return { self_ };
    }
private:
    friend ResourceUnpacker;
    ResourceUnpackerContext(ResourceUnpacker& self)
        : self_(self)
        , task_guard_(self_.cradle_.task_counter)
    {}
    ResourceUnpacker& self_;
    SingleTaskGuard   task_guard_;
};




template<ResourceType TypeV, typename DestinationT,
    of_signature<Job<>(ResourceUnpackerContext, UUID, DestinationT)> UnpackerF>
void ResourceUnpacker::register_unpacker(UnpackerF&& f) {
    const key_type key{ .resource_type=TypeV, .destination_type=typeid(DestinationT) };
    auto [it, was_emplaced] = dispatch_table_.try_emplace(
        key,
        [f=FORWARD(f)](ResourceUnpackerContext context, UUID uuid, AnyRef destination)
            -> Job<>
        {
            // NOTE: We move the destination here because we expect the calling side to
            // pass the reference to a moveable copy of the destination object.
            // Most of the time, the destination is some kind of a handle or a pointer
            // and so this move is superfluous at best.
            return f(MOVE(context), uuid, MOVE(destination.target_unchecked<DestinationT>()));
        }
    );
    assert(was_emplaced);
}


template<ResourceType TypeV, typename DestinationT>
auto ResourceUnpacker::unpack(UUID uuid, DestinationT destination)
    -> Job<>
{
    const key_type key{ .resource_type=TypeV, .destination_type=type_id<DestinationT>() };
    return _unpack(key, uuid, AnyRef(destination));
}


template<typename DestinationT>
auto ResourceUnpacker::unpack_any(UUID uuid, DestinationT destination)
    -> Job<>
{
    const auto type = resource_database_.type_of(uuid);
    const key_type key{ .resource_type=type, .destination_type=type_id<DestinationT>() };
    return _unpack(key, uuid, AnyRef(destination));
}


inline auto ResourceUnpacker::_unpack(const key_type& key, UUID uuid, AnyRef destination)
    -> Job<>
{
    if (auto* kv = try_find(dispatch_table_, key)) {
        // FIXME: Something about const-correctness is amiss here. Likely because of UniqueFunction.
        auto& unpacker = kv->second;
        return unpacker(ResourceUnpackerContext(*this), uuid, destination);
    } else {
        throw RuntimeError(fmt::format("No unpacker found for resource type {} and destination type {}.", resource_info().name_or_id(key.resource_type), key.destination_type.pretty_name()));
    }
}


} // namespace josh
