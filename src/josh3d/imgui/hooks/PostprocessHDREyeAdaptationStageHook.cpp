#include "PostprocessHDREyeAdaptationStageHook.hpp"
#include <imgui.h>





void josh::imguihooks::PostprocessHDREyeAdaptationStageHook::operator()() {

    ImGui::Checkbox("Use Adaptation", &stage_.use_adaptation);

    ImGui::SliderFloat(
        "Adaptation Rate", &stage_.adaptation_rate,
        0.001f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::DragFloat(
        "Screen Value", &stage_.current_screen_value,
        0.5f, 0.0f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    ImGui::DragFloat(
        "Exposure Factor", &stage_.exposure_factor,
        0.5f, 0.0, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic
    );

    int num_samples = static_cast<int>(stage_.num_y_samples);
    if (ImGui::SliderInt(
        "Num Samples", &num_samples,
        1, 1024, "%d", ImGuiSliderFlags_Logarithmic))
    {
        stage_.num_y_samples = num_samples;
    }

}
