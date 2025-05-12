#pragma once
#include "HashedString.hpp"
#include "UUID.hpp"
#include "Usage.hpp"


/*
Basic vocabulary for various resource types.
*/
namespace josh {


template<size_t N>
using ResourceTypeHS = FixedHashedString<N>;
using ResourceType   = HashedID;


/*
Null resource identifier to represent lack of a resource.

NOTE: Default HashedString corresponds to a value of 0.
*/
constexpr auto NullResource = ResourceTypeHS<0>();
static_assert(NullResource.value() == ResourceType());


struct ResourceItem {
    ResourceType type;
    UUID         uuid;
};


using ResourceUsage = Usage<ResourceItem>;


template<ResourceType TypeV> struct resource_traits;


template<ResourceType TypeV>
concept specializes_resource_traits = requires {
    typename resource_traits<TypeV>::resource_type;
};


template<ResourceType TypeV>
struct PublicResource {
    // This should be a "semantically const" resource reference.
    using resource_type = resource_traits<TypeV>::resource_type;
    resource_type resource;
    ResourceUsage usage;
};


// TODO: This should have different semantics.
template<ResourceType TypeV>
using PrivateResource = PublicResource<TypeV>;


} // namespace josh
