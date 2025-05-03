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


struct ResourceItem {
    ResourceType type;
    UUID         uuid;
};


using ResourceUsage = Usage<ResourceItem>;


} // namespace josh
