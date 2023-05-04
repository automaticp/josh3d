#include "ImGuiRegistryHooks.hpp"


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









} // namespace learn
