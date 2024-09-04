#include "LightComponents.hpp"
#include "Tags.hpp"
#include "LightCasters.hpp"
#include "Components.hpp"
#include "Transform.hpp"
#include "tags/ShadowCasting.hpp"
#include "ImGuiHelpers.hpp"
#include <entt/core/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/polar_coordinates.hpp>
#include <imgui.h>




namespace josh::imguihooks::registry {


void LightComponents::operator()(entt::registry& registry) {

    if (ImGui::TreeNode("Ambient")) {

        for (auto [e, ambi] : registry.view<AmbientLight>().each()) {
            ImGui::PushID(void_id(e));
            ImGui::ColorEdit3("Color", glm::value_ptr(ambi.color), ImGuiColorEditFlags_DisplayHSV);
            ImGui::PopID();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Directional")) {

        for (auto [e, dir] : registry.view<DirectionalLight>().each()) {
            ImGui::PushID(void_id(e));
            ImGui::ColorEdit3("Color", glm::value_ptr(dir.color), ImGuiColorEditFlags_DisplayHSV);
            ImGui::SameLine();
            bool has_shadow = registry.all_of<ShadowCasting>(e);
            if (ImGui::Checkbox("Shadow", &has_shadow)) {
                if (has_shadow) {
                    registry.emplace<ShadowCasting>(e);
                } else {
                    registry.remove<ShadowCasting>(e);
                }
            }

            ImGui::PopID();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Point")) {

        thread_local PointLight template_plight{
            .color       = { 1.f, 1.f, 0.8f },
            .attenuation = { .constant = 0.05f, .linear = 0.0f, .quadratic = 0.2f }
        };
        thread_local glm::vec3 template_position{ 0.f };
        thread_local bool      template_has_shadow{ true };

        bool display_node = ImGui::TreeNode("Configure New");
        ImGui::SameLine();
        if (ImGui::SmallButton("Create")) {
            const entt::handle new_plight{ registry, registry.create() };
            new_plight.emplace<PointLight>(template_plight);
            new_plight.emplace<Transform>().translate(template_position);
            if (template_has_shadow) {
                set_tag<ShadowCasting>(new_plight);
            }
        }

        if (display_node) {
            ImGui::DragFloat3("Position", value_ptr(template_position), 0.2f, -FLT_MAX, FLT_MAX);
            ImGui::ColorEdit3("Color",    value_ptr(template_plight.color), ImGuiColorEditFlags_DisplayHSV);
            ImGui::SameLine();
            ImGui::Checkbox("Shadow", &template_has_shadow);
            ImGui::DragFloat3(
                "Atten. (c/l/q)", &template_plight.attenuation.constant,
                0.1f, 0.f, 100.f, "%.4f", ImGuiSliderFlags_Logarithmic
            );

            ImGui::TreePop();
        }
        ImGui::Separator();

        entt::entity to_duplicate{ entt::null };
        entt::entity to_remove{ entt::null };

        for (auto [e, plight] : registry.view<PointLight>().each()) {
            bool display_node =
                ImGui::TreeNode(void_id(e), "Id %d", entt::to_entity(e));

            ImGui::PushID(void_id(e));
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

                ImGui::ColorEdit3("Color", glm::value_ptr(plight.color), ImGuiColorEditFlags_DisplayHSV);
                ImGui::SameLine();
                bool has_shadow = registry.all_of<ShadowCasting>(e);
                if (ImGui::Checkbox("Shadow", &has_shadow)) {
                    if (has_shadow) {
                        registry.emplace<ShadowCasting>(e);
                    } else {
                        registry.remove<ShadowCasting>(e);
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
            const entt::handle old_plight{ registry, to_duplicate      };
            const entt::handle new_plight{ registry, registry.create() };
            copy_components<PointLight, Transform, ShadowCasting>(new_plight, old_plight);
        }

        ImGui::TreePop();
    }

}


} // namespace josh::imguihooks::registry
