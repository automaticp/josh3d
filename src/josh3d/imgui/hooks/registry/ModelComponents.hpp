#pragma once
#include "AssetManager.hpp"
#include <string>
#include <entt/fwd.hpp>


namespace josh::imguihooks::registry {


class ModelComponents {
public:
    ModelComponents(AssetManager& assman);

    void operator()(entt::registry& registry);

private:
    AssetManager& assman_;

    std::string load_path_;
    std::string last_load_error_message_;

    void load_model_widget(entt::registry& registry);
    void model_list_widget(entt::registry& registry);
};


} // namespace josh::imguihooks::registry
