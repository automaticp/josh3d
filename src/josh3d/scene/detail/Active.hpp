#pragma once
#include <entt/entity/fwd.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>


/*
"Active" are things that there's only one of per scene.

These are stored in the registry context.
*/
namespace josh {
namespace detail {


template<typename ActiveT, typename ...RequiredCompTs>
auto get_active(entt::registry& registry)
    -> entt::handle
{
    auto& active = registry.ctx().emplace<ActiveT>();
    if (registry.valid(active.entity) &&
        registry.all_of<RequiredCompTs...>(active.entity))
    {
        return { registry, active.entity };
    }
    return { registry, entt::null };
}


template<typename ActiveT, typename ...RequiredCompTs>
auto get_active(const entt::registry& registry)
    -> entt::const_handle
{
    auto* active = registry.ctx().find<ActiveT>();
    if (active &&
        registry.valid(active->entity) &&
        registry.all_of<RequiredCompTs...>(active->entity))
    {
        return { registry, active->entity };
    }
    return { registry, entt::null };
}


} // namespace detail


// TODO: Should be public interface?
template<typename ActiveT>
auto make_active(entt::handle handle) {
    handle.registry()->ctx().emplace<ActiveT>().entity = handle.entity();
}


} // namespace josh
