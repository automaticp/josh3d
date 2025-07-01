#include "ImGuiEngineHooks.hpp"
#include "PerfHarness.hpp"
#include "Pipeline.hpp"
#include "PipelineStage.hpp"
#include "RenderEngine.hpp"
#include "ScopeExit.hpp"
#include "Time.hpp"
#include "UIContext.hpp"
#include <imgui.h>


namespace josh {


void ImGuiEngineHooks::display(UIContext& ui)
{
    auto& engine   = ui.runtime.renderer;
    auto& hooks    = _hooks;
    auto& pipeline = engine.pipeline;

    const auto display_hooks = [&](auto stages_view)
    {
        for (auto [stage_idx, stage_key] : enumerate(stages_view))
        {
            ImGui::PushID(stage_idx); DEFER(ImGui::PopID());

            Pipeline::StoredStage* stored = pipeline.try_get(stage_key);
            assert(stored);

            auto it = hooks.find(stored->stage.target_type());
            const bool has_hook = it != hooks.end();

            // Show all nodes unconditionally for the GPU|CPU frametimes.
            ImGui::BeginDisabled(not has_hook);
            const bool show_node_contents = ImGui::TreeNode(stored->name.c_str());
            ImGui::EndDisabled();


            PerfHarness* perf_harness = ui.runtime.perf_assembly.try_get(stage_key);

            if (perf_harness)
            {
                // TODO: This should be reimplemented better.
                const float cpu_frametime_text_size = ImGui::CalcTextSize("CPU: 69.42ms").x;
                const float gpu_frametime_text_size = ImGui::CalcTextSize("GPU: 69.42ms |").x;

                const auto& full_segment = *perf_harness->get_segment(0);

                const auto text_duration_ms = [](TimeDeltaNS dt)
                {
                    ImGui::Text("%.2fms", dt.to_seconds<float>() * 1e3f);
                };

                const auto segment_table = [&]()
                {
                    const auto table_flags =
                        ImGuiTableFlags_Borders           |
                        ImGuiTableFlags_Resizable         |
                        ImGuiTableFlags_Reorderable       |
                        ImGuiTableFlags_Hideable          |
                        ImGuiTableFlags_SizingFixedSame   |
                        ImGuiTableFlags_HighlightHoveredColumn;

                    const auto row = [&](
                        const PerfHarness::Segment& s,
                        const char*                 from,
                        const char*                 to)
                    {
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(from);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(to);

                        ImGui::TableNextColumn();
                        text_duration_ms(s.wall_time.current().mean);

                        ImGui::BeginDisabled(not perf_harness->is_gpu_timed());

                        ImGui::TableNextColumn();
                        text_duration_ms(s.host_time.current().mean);

                        ImGui::TableNextColumn();
                        text_duration_ms(s.device_time.current().mean);

                        ImGui::TableNextColumn();
                        text_duration_ms(s.latency.current().mean);

                        ImGui::EndDisabled();
                    };

                    if (ImGui::BeginTable("Segments", 6, table_flags))
                    {
                        DEFER(ImGui::EndTable());

                        // NOTE: Not showing "GPU Host" by default since it's pretty much
                        // the same as the CPU measurements, but redundantly bloats the table.

                        ImGui::TableSetupColumn("From");
                        ImGui::TableSetupColumn("To");
                        ImGui::TableSetupColumn("CPU");
                        ImGui::TableSetupColumn("GPU Host", ImGuiTableColumnFlags_DefaultHide);
                        ImGui::TableSetupColumn("GPU Device");
                        ImGui::TableSetupColumn("Latency");
                        ImGui::TableHeadersRow();

                        // First row for the "full" segment.
                        // TODO: Highlight it somehow?
                        ImGui::TableNextRow();
                        row(full_segment, "start", "end");

                        const auto& snaps = perf_harness->last_frame().snaps;

                        // If there are only 2 snaps, then those are the "start" and "end"
                        // and are already covered by the full segment. So Skip the rest of
                        // the table if so.
                        if (snaps.size() > 2)
                        {
                            for (const uindex istart : irange(snaps.size() - 1))
                            {
                                const uindex iend = istart + 1;
                                const auto* start_segment = perf_harness->get_segment(snaps[istart].id);
                                const auto* end_segment   = perf_harness->get_segment(snaps[iend].id);

                                ImGui::TableNextRow();
                                row(*start_segment, start_segment->name.c_str(), end_segment->name.c_str());
                            }
                        }
                    }
                };

                const auto segments_tooltip = [&]()
                {
                    ImGui::BeginDisabled();
                    bool gpu_timing = perf_harness->is_gpu_timed();
                    // NOTE: Not actually letting you change this because its not generally safe.
                    ImGui::Checkbox("GPU Timing", &gpu_timing);
                    ImGui::EndDisabled();

                    segment_table();
                };

                const float wall_mean_ms   = full_segment.wall_time.current().mean.to_seconds<float>() * 1e3f;
                const float device_mean_ms = full_segment.device_time.current().mean.to_seconds<float>() * 1e3f;

                // FIXME: GetContentRegionMax() seems to be discouraged by the ImGui docs.
                ImGui::SameLine(ImGui::GetContentRegionMax().x - (cpu_frametime_text_size + gpu_frametime_text_size));

                if (perf_harness->is_gpu_timed())
                    ImGui::Text("CPU: %.2fms | GPU: %.2fms", wall_mean_ms, device_mean_ms);
                else
                    ImGui::Text("CPU: %.2fms", wall_mean_ms);

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    segments_tooltip();
                    ImGui::EndTooltip();
                }
            }

            if (show_node_contents)
            {
                // Show the Hook if it exists.
                if (has_hook)
                    it->second(stored->stage.target_as_any());
                ImGui::TreePop();
            }
        }
    };

    if (ImGui::CollapsingHeader("Precompute"))
        display_hooks(pipeline.view(StageKind::Precompute));

    if (ImGui::CollapsingHeader("Primary"))
        display_hooks(pipeline.view(StageKind::Primary));

    if (ImGui::CollapsingHeader("Postprocessing"))
        display_hooks(pipeline.view(StageKind::Postprocess));

    if (ImGui::CollapsingHeader("Overlays"))
        display_hooks(pipeline.view(StageKind::Overlay));
}


} // namespace josh
