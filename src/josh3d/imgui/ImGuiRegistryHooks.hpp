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
public:
    class HooksContainer {
    private:
        friend ImGuiRegistryHooks;
        struct HookEntry {
            HookEntry(UniqueFunction<void(entt::registry&)> hook, std::string name)
                : hook(std::move(hook)), name(std::move(name))
            {}

            UniqueFunction<void(entt::registry& registry)> hook;
            std::string name;
        };

        std::vector<HookEntry> hook_entries_;

    public:
        void add_hook(std::string name, UniqueFunction<void(entt::registry&)> hook) {
            hook_entries_.emplace_back(std::move(hook), std::move(name));
        }
    };

private:
    entt::registry& registry_;
    HooksContainer hooks_container_;

public:
    bool hidden{ false };

    ImGuiRegistryHooks(entt::registry& registry)
        : registry_{ registry }
    {}

    HooksContainer&       hooks() noexcept       { return hooks_container_; }
    const HooksContainer& hooks() const noexcept { return hooks_container_; }

    void display();
};




} // namespace josh
