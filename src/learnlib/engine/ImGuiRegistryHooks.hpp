#pragma once
#include "AssimpModelLoader.hpp"
#include "LightCasters.hpp"
#include "MaterialDSMultilightShadowStage.hpp"
#include "Model.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
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
            .constant = 1.f,
            .linear = 0.4f,
            .quadratic = 0.2f
        }
    }};

    bool plight_has_shadow_{ true };

public:
    void operator()(entt::registry& registry) {

        if (ImGui::TreeNode("Ambient")) {

            for (auto [e, ambi] : registry.view<light::Ambient>().each()) {
                ImGui::PushID(int(e));
                ImGui::ColorEdit3("Color", glm::value_ptr(ambi.color));
                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Directional")) {

            for (auto [e, dir] : registry.view<light::Directional>().each()) {
                ImGui::PushID(int(e));
                ImGui::ColorEdit3("Color", glm::value_ptr(dir.color));
                ImGui::SameLine();
                bool has_shadow = registry.all_of<ShadowComponent>(e);
                if (ImGui::Checkbox("Shadow", &has_shadow)) {
                    if (has_shadow) {
                        registry.emplace<ShadowComponent>(e);
                    } else {
                        registry.remove<ShadowComponent>(e);
                    }
                }
                ImGui::SliderFloat3("Direction", glm::value_ptr(dir.direction), -1.f, 1.f);
                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Point")) {


            bool display_node = ImGui::TreeNode("Configure New");
            ImGui::SameLine();
            if (ImGui::SmallButton("Create")) {
                auto e = registry.create();
                registry.emplace<light::Point>(e, plight_template_);
                if (plight_has_shadow_) {
                    registry.emplace<ShadowComponent>(e);
                }
            }

            if (display_node) {
                ImGui::DragFloat3("Position", glm::value_ptr(plight_template_.position), 0.2f);
                ImGui::ColorEdit3("Color", glm::value_ptr(plight_template_.color));
                ImGui::SameLine();
                ImGui::Checkbox("Shadow", &plight_has_shadow_);
                ImGui::DragFloat3(
                    "Atten. (c/l/q)", &plight_template_.attenuation.constant,
                    0.1f, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic
                );

                ImGui::TreePop();
            }
            ImGui::Separator();

            entt::entity to_duplicate{ entt::null };
            entt::entity to_remove{ entt::null };

            for (auto [e, plight] : registry.view<light::Point>().each()) {
                bool display_node =
                    ImGui::TreeNode(reinterpret_cast<void*>(e), "Id %d", static_cast<entt::id_type>(e));

                ImGui::PushID(reinterpret_cast<void*>(e));
                ImGui::SameLine();
                if (ImGui::SmallButton("Duplicate")) {
                    to_duplicate = e;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove")) {
                    to_remove = e;
                }
                ImGui::PopID();

                if (display_node) {

                    ImGui::DragFloat3("Position", glm::value_ptr(plight.position), 0.2f);

                    ImGui::ColorEdit3("Color", glm::value_ptr(plight.color));

                    ImGui::SameLine();
                    bool has_shadow = registry.all_of<ShadowComponent>(e);
                    if (ImGui::Checkbox("Shadow", &has_shadow)) {
                        if (has_shadow) {
                            registry.emplace<ShadowComponent>(e);
                        } else {
                            registry.remove<ShadowComponent>(e);
                        }
                    }

                    ImGui::DragFloat3(
                        "Atten. (c/l/q)", &plight.attenuation.constant,
                        0.1f, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic
                    );
                    ImGui::TreePop();

                }

            }

            if (to_remove != entt::null) {
                registry.destroy(to_remove);
            }

            if (to_duplicate != entt::null) {
                auto new_e = registry.create();
                registry.emplace<light::Point>(new_e, registry.get<light::Point>(to_duplicate));
                if (registry.all_of<ShadowComponent>(to_duplicate)) {
                    registry.emplace<ShadowComponent>(new_e);
                }
            }

            ImGui::TreePop();
        }

    }
};



class ImGuiRegistryModelComponentsHook {
private:
    std::string load_path;
    std::string last_load_error_message;


public:
    void operator()(entt::registry& registry) {

        if (ImGui::Button("Load")) {
            try {
                Model model = AssimpModelLoader<>().load(load_path).get();

                Shared<Model> model_ptr = std::make_shared<Model>(std::move(model));

                // Wasteful, but whatever for now

                auto e = registry.create();
                registry.emplace<Transform>(e);
                registry.emplace<Shared<Model>>(e, std::move(model_ptr));

                last_load_error_message = {};
            } catch (const error::AssimpLoaderError& e) {
                last_load_error_message = e.what();
            }
        }
        ImGui::SameLine();
        ImGui::InputText("Path", &load_path);

        ImGui::TextUnformatted(last_load_error_message.c_str());

        ImGui::Separator();

        for (auto [e, transform, model] : registry.view<Transform, Shared<Model>>().each()) {
            if (ImGui::TreeNode(reinterpret_cast<void*>(e), "Id %d", static_cast<entt::id_type>(e))) {

                ImGui::DragFloat3(
                    "Position", glm::value_ptr(transform.position()),
                    0.2f, -100.f, 100.f
                );


                // FIXME: This is broken asf, read up on euler -> quat -> euler transformation
                // and how to preserve euler representation consistently.
                // glm::vec3 euler = glm::mod(glm::degrees(glm::eulerAngles(transform.rotation())), 360.f);
                // if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 1.0f, -360.f, 360.f)) {
                //     transform.rotation() = glm::quat(glm::radians(euler));
                // }

                ImGui::SliderFloat4(
                    "Rotation :^", glm::value_ptr(transform.rotation()),
                    -1.f, 1.f
                );

                ImGui::DragFloat3(
                    "Scale", glm::value_ptr(transform.scaling()),
                    0.1f, 0.01f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
                );

                if (ImGui::TreeNode("Materials")) {
                    for (size_t mat_id{ 0 }; auto& drawable : model->drawable_meshes()) {
                        if (ImGui::TreeNode(reinterpret_cast<void*>(mat_id), "Material %zu", mat_id)) {

                            ImGui::Image(
                                reinterpret_cast<ImTextureID>(drawable.material().diffuse->id()),
                                { 256.f, 256.f }
                            );

                            ImGui::Image(
                                reinterpret_cast<ImTextureID>(drawable.material().specular->id()),
                                { 256.f, 256.f }
                            );

                            ImGui::DragFloat(
                                "Shininess", &drawable.material().shininess,
                                1.0f, 0.1f, 1.e4f, "%.3f", ImGuiSliderFlags_Logarithmic
                            );

                            ImGui::TreePop();
                        }

                        ++mat_id;

                    }

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }
        }

    }
};







} // namespace learn
