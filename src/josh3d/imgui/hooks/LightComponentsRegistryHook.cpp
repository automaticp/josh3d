#include "LightComponentsRegistryHook.hpp"
#include "RenderComponents.hpp"
#include "ImGuiHelpers.hpp"
#include <entt/core/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/polar_coordinates.hpp>
#include <imgui.h>



namespace josh::imguihooks {




void LightComponentsRegistryHook::operator()(
    entt::registry& registry)
{

    if (ImGui::TreeNode("Ambient")) {

        for (auto [e, ambi] : registry.view<light::Ambient>().each()) {
            ImGui::PushID(void_id(e));
            ImGui::ColorEdit3("Color", glm::value_ptr(ambi.color), ImGuiColorEditFlags_DisplayHSV);
            ImGui::PopID();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Directional")) {

        for (auto [e, dir] : registry.view<light::Directional>().each()) {
            ImGui::PushID(void_id(e));
            ImGui::ColorEdit3("Color", glm::value_ptr(dir.color), ImGuiColorEditFlags_DisplayHSV);
            ImGui::SameLine();
            bool has_shadow = registry.all_of<components::ShadowCasting>(e);
            if (ImGui::Checkbox("Shadow", &has_shadow)) {
                if (has_shadow) {
                    registry.emplace<components::ShadowCasting>(e);
                } else {
                    registry.remove<components::ShadowCasting>(e);
                }
            }


            // TODO: Might actually make sense to represent direction
            // as theta and phi pair internally. That way, there's no degeneracy.

            // We swap x and y so that phi is rotation around x,
            // and behaves more like the real Sun.
            // We're probably not on the north pole, it's fine.
            glm::vec3 swapped_dir{ dir.direction.y, dir.direction.x, dir.direction.z };
            glm::vec2 polar = glm::degrees(glm::polar(swapped_dir));
            if (ImGui::DragFloat2("Direction", glm::value_ptr(polar), 0.5f)) {
                swapped_dir = glm::euclidean(glm::radians(glm::vec2{ polar.x, polar.y }));
                // Un-swap back.
                dir.direction = glm::vec3{ swapped_dir.y, swapped_dir.x, swapped_dir.z };
            }

            ImGui::BeginDisabled();
            ImGui::InputFloat3("Direction XYZ", glm::value_ptr(dir.direction));
            ImGui::EndDisabled();

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
                registry.emplace<components::ShadowCasting>(e);
            }
        }

        if (display_node) {
            ImGui::DragFloat3("Position", glm::value_ptr(plight_template_.position), 0.2f);
            ImGui::ColorEdit3("Color", glm::value_ptr(plight_template_.color), ImGuiColorEditFlags_DisplayHSV);
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
                ImGui::TreeNode(void_id(e), "Id %d", static_cast<entt::id_type>(e));

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

                ImGui::DragFloat3("Position", glm::value_ptr(plight.position), 0.2f);

                ImGui::ColorEdit3("Color", glm::value_ptr(plight.color), ImGuiColorEditFlags_DisplayHSV);

                ImGui::SameLine();
                bool has_shadow = registry.all_of<components::ShadowCasting>(e);
                if (ImGui::Checkbox("Shadow", &has_shadow)) {
                    if (has_shadow) {
                        registry.emplace<components::ShadowCasting>(e);
                    } else {
                        registry.remove<components::ShadowCasting>(e);
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
            if (registry.all_of<components::ShadowCasting>(to_duplicate)) {
                registry.emplace<components::ShadowCasting>(new_e);
            }
        }

        ImGui::TreePop();
    }

}




} // namespace josh::imguihooks
