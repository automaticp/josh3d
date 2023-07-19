#pragma once
#include <entt/entity/fwd.hpp>
#include <string>




namespace josh::imguihooks {


class SkyboxRegistryHook {
private:
    std::string load_path_;
    std::string filenames_[6];
    std::string error_text_;

public:
    void operator()(entt::registry& registry);

};



} // namespace josh::imguihooks
