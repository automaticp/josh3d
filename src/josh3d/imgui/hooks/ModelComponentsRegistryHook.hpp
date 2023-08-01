#pragma once
#include <string>
#include <entt/fwd.hpp>


namespace josh::imguihooks {


class ModelComponentsRegistryHook {
private:
    std::string load_path_;
    std::string last_load_error_message_;

public:
    void operator()(entt::registry& registry);

private:
    void load_model_widget(entt::registry& registry);
    void model_list_widget(entt::registry& registry);
};


} // namespace josh::imguihooks
