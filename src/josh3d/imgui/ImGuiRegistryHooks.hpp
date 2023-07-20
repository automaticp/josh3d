#pragma once
#include "UniqueFunction.hpp"
#include <entt/entity/fwd.hpp>
#include <string>
#include <utility>


namespace josh {



/*
ImGui container for hooks that interact with the registry.

[Registry]
  [Lights]
    <Your hook here>
  [Models]
    <Your hook here>
  ...

*/
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




} // namespace josh
