#include "ShadowMappingStageHook.hpp"
#include <imgui.h>





void learn::imguihooks::ShadowMappingStageHook::operator()() {
    auto& s = stage_;
    auto& plight_maps = shadow_info_->point_light_maps;
    auto& dir_light_map = shadow_info_->dir_light_map;

    if (ImGui::TreeNode("Point Shadows")) {
        const bool unconfirmed_changes =
            plight_maps.width() != point_shadow_res_;

        const char* label =
            unconfirmed_changes ? "*Apply" : " Apply";

        if (ImGui::Button(label)) {
            stage_.resize_point_maps(
                point_shadow_res_, point_shadow_res_
            );
        }

        ImGui::SliderInt(
            "Resolution", &point_shadow_res_,
            128, 8192, "%d", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat2(
            "Z Near/Far", glm::value_ptr(s.point_params().z_near_far),
            0.01f, 500.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );


        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Directional Shadows")) {
        if (ImGui::TreeNode("Shadow Map")) {

            ImGui::Image(
                reinterpret_cast<ImTextureID>(
                    dir_light_map.depth_target().id()
                ),
                ImVec2{ 300.f, 300.f },
                ImVec2{ 0.f, 1.f }, ImVec2{ 1.f, 0.f }
            );

            ImGui::TreePop();
        }

        const bool unconfirmed_changes =
            dir_light_map.width() != dir_shadow_res_;

        const char* label =
            unconfirmed_changes ? "*Apply" : " Apply";

        if (ImGui::Button(label)) {
            s.resize_dir_map(
                dir_shadow_res_, dir_shadow_res_
            );
        }

        ImGui::SliderInt(
            "Resolution", &dir_shadow_res_,
            128, 8192, "%d", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat(
            "Proj Scale", &s.dir_params().projection_scale,
            0.1f, 10000.f, "%.1f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat2(
            "Z Near/Far", glm::value_ptr(s.dir_params().z_near_far),
            0.001f, 10000.f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat(
            "Cam Offset", &s.dir_params().cam_offset,
            0.1f, 10000.f, "%.1f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }

}
