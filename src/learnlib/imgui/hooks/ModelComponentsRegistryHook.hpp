#pragma once
#include <string>
#include <entt/fwd.hpp>


namespace learn::imguihooks {


class ModelComponentsRegistryHook {
private:
    std::string load_path;
    std::string last_load_error_message;

public:
    void operator()(entt::registry& registry);
};


} // namespace learn::imguihooks
