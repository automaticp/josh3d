#pragma once
#include "LightCasters.hpp"
#include "MaterialDSMultilightShadowStage.hpp"
#include "Model.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include <entt/entt.hpp>
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <entt/entt.hpp>
#include <string>
#include <utility>


namespace learn {


// Just hardcoded for now bc components are not finalized.
class ImGuiRegistryHooks {
private:
    entt::registry& registry_;

public:
    bool hidden{ false };

public:
    ImGuiRegistryHooks(entt::registry& registry)
        : registry_{ registry }
    {}


    void display() {
        if (hidden) { return; }

        ImGui::SetNextWindowSize({ 600.f, 600.f }, ImGuiCond_Once);
        ImGui::SetNextWindowPos({ 0.f, 0.f }, ImGuiCond_Once);
        if (ImGui::Begin("Registry")) {

            if (ImGui::CollapsingHeader("Lights")) {

                if (ImGui::TreeNode("Ambient")) {

                    for (auto [e, ambi] : registry_.view<light::Ambient>().each()) {
                        ImGui::PushID(int(e));
                        ImGui::ColorEdit3("Color", glm::value_ptr(ambi.color));
                        ImGui::PopID();
                    }

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Directional")) {

                    for (auto [e, dir] : registry_.view<light::Directional>().each()) {
                        ImGui::PushID(int(e));
                        ImGui::ColorEdit3("Color", glm::value_ptr(dir.color));
                        ImGui::SliderFloat3("Direction", glm::value_ptr(dir.direction), -1.f, 1.f);
                        ImGui::PopID();
                    }

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Point")) {

                    for (auto [e, plight] : registry_.view<light::Point>().each()) {
                        if (ImGui::TreeNode(reinterpret_cast<void*>(e), "Id %d", static_cast<entt::id_type>(e))) {
                            ImGui::ColorEdit3("Color", glm::value_ptr(plight.color));

                            ImGui::SameLine();
                            bool has_shadow = registry_.all_of<ShadowComponent>(e);
                            if (ImGui::Checkbox("Shadow", &has_shadow)) {
                                if (has_shadow) {
                                    registry_.emplace<ShadowComponent>(e);
                                } else {
                                    registry_.remove<ShadowComponent>(e);
                                }
                            }

                            ImGui::DragFloat3("Position", glm::value_ptr(plight.position), 0.2f);
                            ImGui::DragFloat3(
                                "Atten. (c/l/q)", &plight.attenuation.constant,
                                0.1f, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic
                            );
                            ImGui::TreePop();
                        }
                    }

                    ImGui::TreePop();
                }

            }

            if (ImGui::CollapsingHeader("Models")) {
                for (auto [e, transform, model] : registry_.view<Transform, Shared<Model>>().each()) {
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

                        ImGui::TreePop();
                    }
                }
            }

        }
        ImGui::End();
    }


};








} // namespace learn
