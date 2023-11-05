#pragma once
#include <entt/fwd.hpp>


namespace josh::imguihooks {


class TerrainComponentRegistryHook {
public:
    void operator()(entt::registry& registry);
};


} // namespace josh::imguihooks
