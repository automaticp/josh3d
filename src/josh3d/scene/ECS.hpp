#pragma once
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/handle.hpp>
#include <entt/entity/registry.hpp>


/*
Typedefs over EnTT to ease migration in case the entity type has to change later.

This should be used exclusively for the *scene* registry and associated entities.

Unfortunately, currently not used consistently.
*/
namespace josh {


using Entity   = entt::entity;
using Handle   = entt::handle;
using CHandle  = entt::const_handle;
using Registry = entt::registry;


/*
While entt::null *is* a cute emulation of nullptr, it sadly
does not work well with template deduction where the actual
entity type would be expected.
*/
constexpr auto nullentt = Entity(entt::null);


} // namespace josh
