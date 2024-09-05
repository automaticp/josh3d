#pragma once
#include <entt/entity/fwd.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/handle.hpp>
#include <concepts>


/*
"Active" are things that there can only be one of per scene.

These are stored in the registry context.
*/
namespace josh {
namespace detail {


template<typename T>
concept active_type = requires(T&& active) {
    { active.entity } -> std::convertible_to<entt::entity>;
};


/*
Default generated unique type for each component that satisfies `active_type`.
*/
template<typename T>
struct ActiveFor {
    entt::entity entity{ entt::null };
};


// Like normal `all_of`, but also returns true if the pack is empty.
template<typename ...RequiredTs>
bool has_all_of(entt::const_handle handle) {
    if constexpr (sizeof...(RequiredTs) == 0) {
        return true;
    } else {
        return handle.all_of<RequiredTs...>();
    }
}


template<active_type ActiveT, typename ...RequiredCompTs>
auto get_active_impl(entt::registry& registry)
    -> entt::handle
{
    auto& active = registry.ctx().emplace<ActiveT>();
    entt::handle handle{ registry, active.entity };
    if (handle.valid() && has_all_of<RequiredCompTs...>(handle)) {
        return handle;
    }
    return { registry, entt::null };
}


template<active_type ActiveT, typename ...RequiredCompTs>
auto get_active_impl(const entt::registry& registry)
    -> entt::const_handle
{
    if (auto* active = registry.ctx().find<ActiveT>()) {
        entt::const_handle handle{ registry, active->entity };
        if (handle.valid() && has_all_of<RequiredCompTs...>(handle)) {
            return handle;
        }
    }
    return { registry, entt::null };
}


} // namespace detail




// Returns an active object for `PrimaryComponentT`, if possible.
//
// Returns a null handle if there's no active entity for `PrimaryComponentT`.
// Returns a null handle if the active entity does not have `OtherRequiredTs`.
template<typename PrimaryComponentT, typename ...OtherRequiredTs>
[[nodiscard]] auto get_active(entt::registry& registry)
    -> entt::handle
{
    return detail::get_active_impl<detail::ActiveFor<PrimaryComponentT>, OtherRequiredTs...>(registry);
}


// Returns an active object for `PrimaryComponentT`, if possible.
//
// Returns a null handle if there's no active entity for `PrimaryComponentT`.
// Returns a null handle if the active entity does not have `OtherRequiredTs`.
template<typename PrimaryComponentT, typename ...OtherRequiredTs>
[[nodiscard]] auto get_active(const entt::registry& registry)
    -> entt::const_handle
{
    return detail::get_active_impl<detail::ActiveFor<PrimaryComponentT>, OtherRequiredTs...>(registry);
}


// Makes an entity active for the `PrimaryComponentT`.
template<typename PrimaryComponentT>
void make_active(entt::handle handle) {
    handle.registry()->ctx().emplace<detail::ActiveFor<PrimaryComponentT>>().entity = handle.entity();
}


template<typename PrimaryComponentT>
bool has_active(const entt::registry& registry) {
    if (const auto* active = registry.ctx().find<detail::ActiveFor<PrimaryComponentT>>()) {
        return registry.valid(active->entity);
    }
    return false;
}


template<typename PrimaryComponentT>
bool is_active(entt::const_handle handle) {
    return handle == get_active<PrimaryComponentT>(*handle.registry());
}


} // namespace josh
