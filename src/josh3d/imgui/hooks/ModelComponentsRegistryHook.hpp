#pragma once
#include <string>
#include <entt/fwd.hpp>


namespace josh::imguihooks {


class ModelComponentsRegistryHook {
private:
    std::string load_path;
    std::string last_load_error_message;

public:
    void operator()(entt::registry& registry);
};


} // namespace josh::imguihooks
