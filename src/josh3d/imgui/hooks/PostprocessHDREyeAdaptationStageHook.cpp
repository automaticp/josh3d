#include "PostprocessHDREyeAdaptationStageHook.hpp"
#include <imgui.h>





void josh::imguihooks::PostprocessHDREyeAdaptationStageHook::operator()() {

    ImGui::Checkbox("Use Adaptation", &stage_.use_adaptation);

    if (ImGui::TreeNode("Adjust Screen Value (Slow)")) {

        float val = stage_.get_screen_value();
        if (ImGui::DragFloat("Screen Value", &val,
            0.5f, 0.0f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic))
        {
            stage_.set_screen_value(val);
        }

        ImGui::TreePop();
    }

    ImGui::SliderFloat(
        "Adaptation Rate", &stage_.adaptation_rate,
        0.001f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::DragFloat(
        "Exposure Factor", &stage_.exposure_factor,
        0.5f, 0.0, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    int num_y_samples = static_cast<int>(stage_.num_y_sample_blocks);
    if (ImGui::SliderInt(
        "Num Y Sample Blocks", &num_y_samples,
        1, 1024, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.num_y_sample_blocks = num_y_samples;
    }

    auto dims = stage_.get_sampling_block_dims();
    ImGui::Text(
        "Num Blocks: (%lu, %lu)[%lu]",
        dims.width, dims.height, dims.area()
    );

    ImGui::Text("Block Size: (8, 8)[64]");

    ImGui::Text(
        "Num Samples: (%lu, %lu)[%lu]",
        dims.width * 8, dims.height * 8, dims.area() * 64
    );


}
