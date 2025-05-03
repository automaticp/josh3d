#pragma once
#include <entt/core/hashed_string.hpp>


/*
A simple reexport of EnTT functionality.

NOTE: I think it would be helpful if the integer type of the HashedString was
customizable, so that we could use separate strong ints/enum classes
for declaring string identifiers for different purposes, like:

    enum class ResourceType    : uint32_t {}; // Intentionally empty.
    enum class SceneObjectType : uint32_t {}; // Intentionally empty.

    constexpr auto Camera   = HashedString<SceneObjectType>("Camera"); // No corresponding resource.
    constexpr auto Skeleton = HashedString<ResourceType>("Skeleton");  // No corresponding scene object.

    constexpr auto Mesh    = HashedString<ResourceType>("Mesh");    // Exactly the same underlying values.
    constexpr auto MeshObj = HashedString<SceneObjectType>("Mesh"); // Meaning can value-cast between if needed.

*/
namespace josh {

// Not sure if useful, but I'll let it be.
using entt::literals::operator""_hs;

// Not really intended as a primary interface, best to declare
// a constexpr variable with the string value instead.
using HashedString = entt::hashed_string;

// For use in hash tables and such.
// Can have a collision of definitions, but should be rare.
using HashedID = entt::hashed_string::hash_type;

} // namespace josh
