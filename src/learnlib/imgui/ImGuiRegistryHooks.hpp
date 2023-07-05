#pragma once
#include "LightCasters.hpp"
#include "UniqueFunction.hpp"
#include <entt/entity/fwd.hpp>
#include <string>
#include <utility>


namespace learn {




class ImGuiRegistryHooks {
private:
    entt::registry& registry_;

    struct HookEntry {
        HookEntry(UniqueFunction<void(entt::registry&)> hook, std::string name)
            : hook(std::move(hook)), name(std::move(name))
        {}

        UniqueFunction<void(entt::registry& registry)> hook;
        std::string name;
    };

    std::vector<HookEntry> hooks_;

public:
    bool hidden{ false };

public:
    ImGuiRegistryHooks(entt::registry& registry)
        : registry_{ registry }
    {}

    void add_hook(std::string name, UniqueFunction<void(entt::registry&)> hook) {
        hooks_.emplace_back(std::move(hook), std::move(name));
    }

    void display();
};




class ImGuiRegistryLightComponentsHook {
private:
    light::Point plight_template_{ light::Point{
        .color = { 1.f, 1.f, 0.8f },
        .position = { 0.0f, 1.f, 0.f },
        .attenuation = light::Attenuation{
            .constant = 0.05f,
            .linear = 0.0f,
            .quadratic = 0.2f
        }
    }};

    bool plight_has_shadow_{ true };

public:
    void operator()(entt::registry& registry);
};




class ImGuiRegistryModelComponentsHook {
private:
    std::string load_path;
    std::string last_load_error_message;

public:
    void operator()(entt::registry& registry);
};







} // namespace learn
