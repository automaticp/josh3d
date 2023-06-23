#include "ImGuiRegistryHooks.hpp"
#include "AssimpModelLoader.hpp"
#include "Model.hpp"
#include "Shared.hpp"
#include "Transform.hpp"
#include "RenderComponents.hpp"
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>


namespace learn {



void ImGuiRegistryLightComponentsHook::operator()(entt::registry& registry) {

    if (ImGui::TreeNode("Ambient")) {

        for (auto [e, ambi] : registry.view<light::Ambient>().each()) {
            ImGui::PushID(int(e));
            ImGui::ColorEdit3("Color", glm::value_ptr(ambi.color), ImGuiColorEditFlags_DisplayHSV);
            ImGui::PopID();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Directional")) {

        for (auto [e, dir] : registry.view<light::Directional>().each()) {
            ImGui::PushID(int(e));
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




void ImGuiRegistryModelComponentsHook::operator()(entt::registry& registry) {

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


            // ImGui::BeginDisabled();
            // ImGui::DragFloat4("Quaternion", glm::value_ptr(transform.rotation()));
            // ImGui::EndDisabled();

            // FIXME: This is slightly more usable, but the singularity for Pitch around 90d
            // is still unstable. In general: Local X is Pitch, Global Y is Yaw, and Local Z is Roll.
            // Stll very messy to use, but should get the ball rolling.
            const glm::quat& q = transform.rotation();
            // Swap quaternion axes to make pitch around (local) X axis.
            // Also GLM for some reason assumes that the locking [-90, 90] axis is
            // associated with Yaw, not Pitch...? Maybe I am confused here but
            // I want it Pitch, so we also have to swap the euler representation.
            // (In my mind, Pitch and Yaw are Theta and Phi in spherical coordinates respectively).
            const glm::quat q_shfl = glm::quat{ q.w, q.y, q.x, q.z };
            glm::vec3 euler = glm::degrees(glm::vec3{
                glm::yaw(q_shfl),   // Pitch
                glm::pitch(q_shfl), // Yaw
                glm::roll(q_shfl)   // Roll
                // Dont believe what GLM says
            });
            if (ImGui::DragFloat3("Pitch/Yaw/Roll", glm::value_ptr(euler), 1.0f, -360.f, 360.f, "%.3f")) {
                euler.x = glm::clamp(euler.x, -89.999f, 89.999f);
                euler.y = glm::mod(euler.y, 360.f);
                euler.z = glm::mod(euler.z, 360.f);
                // Un-shuffle back both the euler angles and quaternions.
                glm::quat p = glm::quat(glm::radians(glm::vec3{ euler.y, euler.x, euler.z }));
                transform.rotation() = glm::quat{ p.w, p.y, p.x, p.z };
            }

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









} // namespace learn
