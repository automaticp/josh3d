#include "DeferredShading.hpp"
#include "detail/SimpleStageHookMacro.hpp"
#include "stages/primary/DeferredShading.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, DeferredShading) {

    using Mode = stages::primary::DeferredShading::Mode;

    if (ImGui::RadioButton("Singlepass", stage_.mode == Mode::SinglePass)) {
        stage_.mode = Mode::SinglePass;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Multipass", stage_.mode == Mode::MultiPass)) {
        stage_.mode = Mode::MultiPass;
    }


    if (ImGui::TreeNode("Ambient Occlusion")) {

        ImGui::Checkbox("Use Ambient Occlusion", &stage_.use_ambient_occlusion);

        ImGui::SliderFloat(
            "AO Power", &stage_.ambient_occlusion_power,
            0.01f, 100.f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Point Lights/Shadows")) {

        ImGui::SliderFloat(
            "Fade Start", &stage_.plight_fade_start_fraction,
            0.0, 1.0, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderFloat(
            "Fade Length", &stage_.plight_fade_length_fraction,
            0.0, 1.0, "%.3f", ImGuiSliderFlags_Logarithmic
        );


        ImGui::SliderFloat2(
            "Shadow Bias",
            glm::value_ptr(stage_.point_params.bias_bounds),
            0.00001f, 0.5f, "%.5f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::SliderInt(
            "PCF Extent", &stage_.point_params.pcf_extent, 0, 6
        );

        ImGui::SliderFloat(
            "PCF Offset", &stage_.point_params.pcf_offset,
            0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("CSM Shadows")) {

        ImGui::SliderFloat(
            "Base Bias, tx",
            &stage_.dir_params.base_bias_tx,
            0.01f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic
        );


        ImGui::Checkbox("Blend Cascades", &stage_.dir_params.blend_cascades);

        ImGui::SliderFloat(
            "Blend, inner tx",
            &stage_.dir_params.blend_size_inner_tx,
            0.1f, 1000.f, "%.1f", ImGuiSliderFlags_Logarithmic
        );


        ImGui::SliderInt(
            "PCF Extent", &stage_.dir_params.pcf_extent, 0, 12
        );

        ImGui::SliderFloat(
            "PCF Offset, tx", &stage_.dir_params.pcf_offset,
            0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic
        );

        ImGui::TreePop();
    }


}

