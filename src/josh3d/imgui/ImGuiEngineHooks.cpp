#include "ImGuiEngineHooks.hpp"
#include "RenderEngine.hpp"
#include "RenderStage.hpp"
#include "ImGuiApplicationAssembly.hpp"
#include "UIContext.hpp"
#include <imgui.h>


namespace josh {


void ImGuiEngineHooks::display(UIContext& ui)
{
    auto& engine = ui.runtime.renderer;
    auto& hooks  = _hooks;
    auto display_hooks = [&](auto stages_view)
    {
        for (int id = 0; auto& stage : stages_view) {
            ImGui::PushID(id);

            auto it = hooks.find(stage.stage_type_index());
            const bool has_hook = it != hooks.end();

            // Show all nodes unconditionally for the GPU|CPU frametimes.
            ImGui::BeginDisabled(not has_hook);
            const bool show_node_contents = ImGui::TreeNode(stage.name().c_str());
            ImGui::EndDisabled();

            const float cpu_frametime_text_size = ImGui::CalcTextSize("CPU: 69.42ms").x;
            const float gpu_frametime_text_size = ImGui::CalcTextSize("GPU: 69.42ms |").x;

            ImGui::BeginDisabled(not engine.capture_stage_timings);
            ImGui::SameLine(ImGui::GetContentRegionMax().x - (cpu_frametime_text_size + gpu_frametime_text_size));
            ImGui::Text("GPU: %.2fms | ", stage.gpu_frametimer().get_current_average() * 1.e3f);
            ImGui::SameLine(ImGui::GetContentRegionMax().x - cpu_frametime_text_size);
            ImGui::Text("CPU: %.2fms", stage.cpu_frametimer().get_current_average() * 1.e3f);
            ImGui::EndDisabled();

            if (show_node_contents)
            {
                // Show the Hook if it exists.
                if (has_hook)
                    it->second(stage.get().target_as_any());
                ImGui::TreePop();
            }

            ImGui::PopID();
            ++id;
        }
    };

    if (ImGui::CollapsingHeader("Precompute"))
        display_hooks(engine.precompute_stages_view());

    if (ImGui::CollapsingHeader("Primary"))
        display_hooks(engine.primary_stages_view());

    if (ImGui::CollapsingHeader("Postprocessing"))
        display_hooks(engine.postprocess_stages_view());

    if (ImGui::CollapsingHeader("Overlays"))
        display_hooks(engine.overlay_stages_view());
}


} // namespace josh
