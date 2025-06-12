#pragma once
#include "Common.hpp"
#include "TypeInfo.hpp"
#include "ContainerUtils.hpp"
#include "Resource.hpp"
#include "StringHash.hpp"
#include <functional>
#include <ranges>


namespace josh {


class ResourceInfo;

/*
Access the global ResourceInfo instance.

NOTE: Since ResourceInfo is just meta information about resource types that
is usually registered once in the beginning of the program I feel like using
global state here is a reasonable tradeoff compared to threading the reference
to ResourceInfo through every constructor.

All mutable global state is a PITA when it can alter the output or control flow
of the program, however, in this case most of the state difference is whether
the resource has been registered w.r.t. some desired meta property. That is it,
no other internal state is modified. Most of the time the cause for this is
simply a bug made by the programmer.

Plus other runtime meta frameworks use global tables just fine.
I mean, the RTTI is global too. Just resolved at compile/link time.
*/
auto resource_info() noexcept
    -> ResourceInfo&;

/*
A simple container of the resource meta information.

HMM: It could be useful to accept arbitrary properties
encoded by their type. A table of tables, so to speak.
*/
class ResourceInfo
{
public:
    // Register the resource type from the hashed string identifier.
    // This will pickup the name from the hashed string and the
    // resource_type from the resource_traits.
    template<ResourceTypeHS TypeHS>
    auto register_resource_type()
        -> bool
    {
        static_assert(TypeHS.size(), "Resource must have a non-empty name.");
        return register_resource_type<ResourceType(TypeHS)>({ TypeHS.data(), TypeHS.size() });
    }

    // Register the resource type under the specified runtime name.
    // This will pickup the resource_type from the resource traits.
    template<ResourceType TypeV> requires specializes_resource_traits<TypeV>
    auto register_resource_type(StrView name)
        -> bool
    {
        using resource_type = resource_traits<TypeV>::resource_type;
        assert(name.size() && "Resource must have a non-empty name.");
        auto [it, was_emplaced] =
            id2info_.try_emplace(TypeV, defer_explicit(name), type_id<resource_type>().type_info());
        if (not was_emplaced) return false;
        name2id_.try_emplace(name, TypeV);
        return true;
    }

    // Returns a ResourceType view for all registered resource types.
    auto view_registered() const noexcept
        -> std::ranges::view auto
    {
        return id2info_ | std::views::keys;
    }

    auto name_of(ResourceType type) const noexcept
        -> StrView
    {
        if (const auto* info = try_find_value(id2info_, type))
            return info->name;
        else
            return {};
    }

    auto name_or(ResourceType type, StrView default_) const noexcept
        -> StrView
    {
        if (const auto name = name_of(type); name.size())
            return name;
        else
            return default_;
    }

    // Returns the name of the resource or a stringified version
    // of the `type` argument if no such resource is registered.
    // Possibly slow, but convenient.
    auto name_or_id(ResourceType type) const noexcept
        -> String
    {
        if (const auto name = name_of(type); name.size())
            return String(name);
        else
            return std::to_string(type);
    }

    auto type_of(ResourceType type) const noexcept
        -> const TypeInfo*
    {
        if (const auto* info = try_find_value(id2info_, type))
            return &info->resource_type;
        else
            return nullptr;
    }

    // Returns the ResourceType corresponding to the resource name
    // or NullResource if no resource with such name is registered.
    auto id_from_name(StrView name)
        -> ResourceType
    {
        if (const auto* id = try_find_value(name2id_, name))
            return *id;
        else
            return NullResource;
    }

private:
    struct Info
    {
        String          name;
        const TypeInfo& resource_type;
    };

    using id2info_map_type = HashMap<ResourceType, Info>;
    using name2id_map_type = HashMap<String, ResourceType, string_hash, std::equal_to<>>;
    id2info_map_type id2info_;
    name2id_map_type name2id_;
};

inline auto resource_info() noexcept
    -> ResourceInfo&
{
    static ResourceInfo instance;
    return instance;
}


} // namespace josh
