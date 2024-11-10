#include "ImGuiApplicationAssembly.hpp"
#include "Active.hpp"
#include "FrameTimer.hpp"
#include "ImGuiHelpers.hpp"
#include "ImGuiSceneList.hpp"
#include "ImGuiSelected.hpp"
#include "ImGuizmoGizmos.hpp"
#include "Camera.hpp"
#include "RenderEngine.hpp"
#include "Size.hpp"
#include "Transform.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>
#include <cstdio>
#include <iterator>




namespace josh {


ImGuiApplicationAssembly::ImGuiApplicationAssembly(
    glfw::Window&      window,
    RenderEngine&      engine,
    entt::registry&    registry,
    SceneImporter&     importer,
    VirtualFilesystem& vfs
)
    : window_         { window             }
    , engine_         { engine             }
    , registry_       { registry           }
    , importer_       { importer           }
    , vfs_            { vfs                }
    , context_        { window             }
    , window_settings_{ window             }
    , vfs_control_    { vfs                }
    , stage_hooks_    { engine             }
    , scene_list_     { registry, importer }
    , selected_menu_  { registry           }
    , gizmos_         { registry           }
{}


ImGuiIOWants ImGuiApplicationAssembly::get_io_wants() const noexcept {
    auto& io = ImGui::GetIO();

    return {
        .capture_mouse     = io.WantCaptureMouse,
        .capture_mouse_unless_popup_close
                           = io.WantCaptureMouseUnlessPopupClose,
        .capture_keyboard  = io.WantCaptureKeyboard,
        .text_input        = io.WantTextInput,
        .set_mouse_pos     = io.WantSetMousePos,
        .save_ini_settings = io.WantSaveIniSettings
    };
}


void ImGuiApplicationAssembly::new_frame() {
    // FIXME: Use external FrameTimer.
    avg_frame_timer_.update(globals::frame_timer.delta<float>());

    std::snprintf(
        fps_str_.data(), fps_str_.size() + 1,
        fps_str_fmt_,
        1.f / avg_frame_timer_.get_current_average()
    );

    std::snprintf(
        frametime_str_.data(), frametime_str_.size() + 1,
        frametime_str_fmt_,
        avg_frame_timer_.get_current_average() * 1.E3f // s -> ms
    );

    std::snprintf(
        gizmo_info_str_.data(), gizmo_info_str_.size() + 1,
        gizmo_info_str_fmt_,
        [this]() -> char {
            switch (active_gizmo_space()) {
                using enum GizmoSpace;
                case World: return 'W';
                case Local: return 'L';
                default:    return ' ';
            }
        }(),
        [this]() -> char {
            switch (active_gizmo_operation()) {
                using enum GizmoOperation;
                case Translation: return 'T';
                case Rotation:    return 'R';
                case Scaling:     return 'S';
                default:          return ' ';
            }
        }()
    );

    context_.new_frame();
    gizmos_.new_frame();
}


void ImGuiApplicationAssembly::draw_widgets() {

    // TODO: Keep active windows within docknodes across "hides".

    auto bg_col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    bg_col.w = 0.f;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);
    // FIXME: This is probably terribly broken in some way.
    // For the frames that reset the dockspace, this initialization
    // may be reset before OR after the widgets are drawn:
    // in the direct call to reset_dockspace() on resize,
    // or on_value_change_from() after the widget scope.
    // How it still works somewhat "correctly" after both,
    // is beyond me.
    const ImGuiID dockspace_id = 1; // TODO: Is this an arbitraty nonzero value? IDK after the API change.
    ImGui::DockSpaceOverViewport(
        dockspace_id,
        ImGui::GetMainViewport(),
        ImGuiDockNodeFlags_PassthruCentralNode
    );
    ImGui::PopStyleColor();

    // FIXME: Terrible, maybe will add "was resized" flag to WindowSizeCache instead.
    static Extent2F old_size{ 0, 0 };
    auto vport_size = ImGui::GetMainViewport()->Size;
    Extent2F new_size = { vport_size.x, vport_size.y };
    if (old_size != new_size) {
        // Do the reset inplace, before the windows are submitted.
        reset_dockspace(dockspace_id);
        old_size = new_size;
    }

    thread_local bool show_demo_window = false;

    if (!is_hidden()) {


        auto reset_condition = on_value_change_from(false, [&, this] { reset_dockspace(dockspace_id); });

        auto bg_col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        bg_col.w = background_alpha;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);


        if (ImGui::BeginMainMenuBar()) {
            ImGui::TextUnformatted("Josh3D-Demo");

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);


            if (ImGui::BeginMenu("Window")) {
                window_settings_.display();
                ImGui::EndMenu();
            }


            if (ImGui::BeginMenu("ImGui")) {

                ImGui::Checkbox("Show Demo Window", &show_demo_window);

                ImGui::SliderFloat(
                    "FPS Avg. Interval, s", &avg_frame_timer_.averaging_interval,
                    0.001f, 5.f, "%.3f", ImGuiSliderFlags_Logarithmic
                );

                ImGui::SliderFloat("Bg. Alpha", &background_alpha, 0.f, 1.f);

                reset_condition.set(ImGui::Button("Reset Dockspace"));

                ImGui::Checkbox("Gizmo Debug Window", &gizmos_.display_debug_window);

                const char* gizmo_locations[] = {
                    "Local Origin",
                    "AABB Midpoint"
                };

                int location_id = to_underlying(gizmos_.preferred_location);
                if (ImGui::ListBox("Gizmo Locaton", &location_id,
                    gizmo_locations, std::size(gizmo_locations), 2))
                {
                    gizmos_.preferred_location = GizmoLocation{ location_id };
                }

                ImGui::Checkbox("Show Model Matrix in Selected", &selected_menu_.display_model_matrix);

                ImGui::EndMenu();
            }


            if (ImGui::BeginMenu("Engine")) {

                ImGui::Checkbox("RGB -> sRGB",    &engine_.enable_srgb_conversion);
                ImGui::Checkbox("GPU/CPU Timers", &engine_.capture_stage_timings );

                ImGui::BeginDisabled(!engine_.capture_stage_timings);
                ImGui::SliderFloat(
                    "Timing Interval, s", &engine_.stage_timing_averaging_interval_s,
                    0.001f, 5.f, "%.3f", ImGuiSliderFlags_Logarithmic
                );
                ImGui::EndDisabled();

                using HDRFormat = RenderEngine::HDRFormat;

                const HDRFormat formats[3]{
                    HDRFormat::R11F_G11F_B10F,
                    HDRFormat::RGB16F,
                    HDRFormat::RGB32F
                };

                const char* format_names[3]{
                    "R11F_G11F_B10F",
                    "RGB16F",
                    "RGB32F",
                };

                size_t current_idx = 0;
                for (const HDRFormat format : formats) {
                    if (format == engine_.main_buffer_format) { break; }
                    ++current_idx;
                }

                if (ImGui::BeginCombo("HDR Format", format_names[current_idx])) {
                    for (size_t i{ 0 }; i < std::size(formats); ++i) {
                        if (ImGui::Selectable(format_names[i], current_idx == i)) {
                            engine_.main_buffer_format = formats[i];
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::EndMenu();
            }


            if (ImGui::BeginMenu("VFS")) {
                vfs_control_.display();
                ImGui::EndMenu();
            }


            {
                auto log_view = log_sink_.view();
                const bool new_logs = log_view.size() > last_log_size_;

                if (new_logs) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.0, 1.0, 0.0, 1.0 });
                }

                // NOTE: This is somewhat messy. If this is common, might be worth writing helpers.
                thread_local bool logs_open_b4 = false;
                const bool        logs_open    = ImGui::BeginMenu("Logs");

                if (new_logs) {
                    ImGui::PopStyleColor();
                }

                const bool was_closed = !logs_open && logs_open_b4;

                if (logs_open) {
                    ImGui::TextUnformatted(log_view.begin(), log_view.end());
                    ImGui::EndMenu();
                }

                if (was_closed) {
                    last_log_size_ = log_view.size();
                }

                logs_open_b4 = logs_open;
            }


            const float size_gizmo     = ImGui::CalcTextSize(gizmo_info_str_template_).x;
            const float size_fps       = ImGui::CalcTextSize(fps_str_template_).x;
            const float size_frametime = ImGui::CalcTextSize(frametime_str_template_).x;

            ImGui::SameLine(ImGui::GetContentRegionMax().x - (size_gizmo + size_fps + size_frametime));
            ImGui::TextUnformatted(gizmo_info_str_.c_str());
            ImGui::SameLine(ImGui::GetContentRegionMax().x - (size_fps + size_frametime));
            ImGui::TextUnformatted(fps_str_.c_str());
            ImGui::SameLine(ImGui::GetContentRegionMax().x - (size_frametime));
            ImGui::TextUnformatted(frametime_str_.c_str());

            ImGui::EndMainMenuBar();
        }


        if (ImGui::Begin("Render Engine")) {
            stage_hooks_.display();
        } ImGui::End();

        if (ImGui::Begin("Selected")) {
            selected_menu_.display();
        } ImGui::End();

        if (ImGui::Begin("Scene")) {
            scene_list_.display();
        } ImGui::End();

        if (show_demo_window) {
            ImGui::ShowDemoWindow();
        }

        ImGui::PopStyleColor();
    }

}


void ImGuiApplicationAssembly::display() {
    draw_widgets();
    if (const auto camera = get_active<Camera, MTransform>(registry_)) {
        const glm::mat4 view_mat = inverse(camera.get<MTransform>().model());
        const glm::mat4 proj_mat = camera.get<Camera>().projection_mat();
        gizmos_.display(view_mat, proj_mat);
    }
    context_.render();
}


void ImGuiApplicationAssembly::reset_dockspace(ImGuiID dockspace_id) {
    ImGui::DockBuilderRemoveNode(dockspace_id);
    auto flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags{ ImGuiDockNodeFlags_DockSpace };
	ImGui::DockBuilderAddNode(dockspace_id, flags);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    float h_split = 3.5f;
    auto left_id        = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left,  1.f / h_split, nullptr, &dockspace_id);
    h_split -= 1.f;
    auto right_id       = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 1.f / h_split, nullptr, &dockspace_id);
    auto left_bottom_id = ImGui::DockBuilderSplitNode(left_id,      ImGuiDir_Down,  0.5f,          nullptr, &left_id     );

    ImGui::DockBuilderDockWindow("Selected",      left_bottom_id);
    ImGui::DockBuilderDockWindow("Scene",         left_id       );
    ImGui::DockBuilderDockWindow("Render Engine", right_id      );


    ImGui::DockBuilderFinish(dockspace_id);
}


} // namespace josh
