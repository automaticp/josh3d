#pragma once
#include "AssimpModelLoader.hpp"
#include "LightCasters.hpp"
#include "Model.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include "UniqueFunction.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <entt/entt.hpp>
#include <imgui_stdlib.h>
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


    void display() {
        if (hidden) { return; }

        ImGui::SetNextWindowSize({ 600.f, 600.f }, ImGuiCond_Once);
        ImGui::SetNextWindowPos({ 0.f, 0.f }, ImGuiCond_Once);
        if (ImGui::Begin("Registry")) {

            for (size_t i{ 0 }; i < hooks_.size(); ++i) {

                ImGui::PushID(int(i));
                if (ImGui::CollapsingHeader(hooks_[i].name.c_str())) {

                    hooks_[i].hook(registry_);

                }
                ImGui::PopID();

            }
        } ImGui::End();
    }


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
