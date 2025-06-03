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


/*
Unique identifier of each resource.

TODO: Might not need to store the type. Bloats sizes, can be recovered through database.
*/
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


/*
Resource-with-usage that provides a read-only "public" access to a particular resource.

At least in theory. Currently does not enforce the read-only property as making "const"
versions of each resource type is tedious and sometimes impossible without heaps of
wrappers. This is similar to the const_iterator problem, since most of the resource
types are simple handles. Right now, "read-only" is only ensured by an honor system.
*/
template<ResourceType TypeV>
struct PublicResource {
    using resource_type = resource_traits<TypeV>::resource_type;
    resource_type resource;
    ResourceUsage usage;
};


/*
Resource-with-usage that provides mutable "private" access to a particular resource.

Intended to be used by the resource registry, loaders and other "internal" moving
parts. Might have different usage semantics to exclude self from LRU logic (not done yet).
*/
template<ResourceType TypeV>
using PrivateResource = PublicResource<TypeV>;


} // namespace josh
