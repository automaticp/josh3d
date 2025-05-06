#pragma once
#include "HashedString.hpp"
#include "UUID.hpp"
#include "Usage.hpp"


/*
Basic vocabulary for various resource types.
*/
namespace josh {


using ResourceType   = HashedID;
using ResourceTypeHS = HashedString;


/*
Null resource identifier to represent lack of a resource.

NOTE: Default HashedString corresponds to a value of 0.
*/
constexpr auto NullResource = HashedString();
static_assert(NullResource.value() == ResourceType());


struct ResourceItem {
    ResourceType type;
    UUID         uuid;
};


using ResourceUsage = Usage<ResourceItem>;


} // namespace josh
