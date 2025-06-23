#include "ImGuiApplicationAssembly.hpp"
#include "Active.hpp"
#include "Asset.hpp"
#include "AssetManager.hpp"
#include "AsyncCradle.hpp"
#include "CategoryCasts.hpp"
#include "ContainerUtils.hpp"
#include "FrameTimer.hpp"
#include "GLAPIBinding.hpp"
#include "ID.hpp"
#include "ImGuiExtras.hpp"
#include "ImGuiHelpers.hpp"
#include "ImGuiResourceViewer.hpp"
#include "ImGuiSceneList.hpp"
#include "ImGuiSelected.hpp"
#include "ImGuizmoGizmos.hpp"
#include "Camera.hpp"
#include "Materials.hpp"
#include "Ranges.hpp"
#include "RenderEngine.hpp"
#include "Region.hpp"
#include "SkeletonStorage.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "VirtualFilesystem.hpp"
#include "glfwpp/window.h"
#include <fmt/core.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <range/v3/view/enumerate.hpp>
#include <exception>
#include <cmath>
#include <cstdio>
#include <iterator>


namespace josh {


ImGuiApplicationAssembly::ImGuiApplicationAssembly(
    glfw::Window& window,
    Runtime&      runtime
)
    : window(window)
    , runtime(runtime)
    , imgui_context(window)
{}

auto ImGuiApplicationAssembly::get_io_wants() const noexcept
    -> ImGuiIOWants
{
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

void ImGuiApplicationAssembly::new_frame(const FrameTimer& frame_timer)
{
    const float dt = frame_timer.delta<float>();
    _avg_frame_timer.update(dt);

    _frame_deltas.resize(_num_frames_plotted);
    _frame_offset = (_frame_offset + 1) % int(_frame_deltas.size());
    _frame_deltas.at(_frame_offset) = dt * 1.e3f; // Convert to ms.

    std::snprintf(_fps_str.data(), _fps_str.size() + 1,
        _fps_str_fmt, 1.f / _avg_frame_timer.get_current_average());

    std::snprintf(_frametime_str.data(), _frametime_str.size() + 1,
        _frametime_str_fmt, _avg_frame_timer.get_current_average() * 1.E3f); // s -> ms

    const char space_char = eval%[this]{
        switch (gizmos.active_space)
        {
            using enum GizmoSpace;
            case World: return 'W';
            case Local: return 'L';
            default:    return ' ';
        }
    };

    const char op_char = eval%[this]{
        switch (gizmos.active_operation)
        {
            using enum GizmoOperation;
            case Translation: return 'T';
            case Rotation:    return 'R';
            case Scaling:     return 'S';
            default:          return ' ';
        }
    };

    std::snprintf(_gizmo_info_str.data(), _gizmo_info_str.size() + 1,
        _gizmo_info_str_fmt, space_char, op_char);

    imgui_context.new_frame();
    gizmos.new_frame();
}

namespace {

// TODO: Deprecate
void display_asset_manager_debug(AssetManager& asset_manager)
{
    thread_local Optional<SharedTextureAsset>            texture_asset;
    thread_local Optional<SharedJob<SharedTextureAsset>> last_texture_job;
    thread_local String                                  texture_vpath;

    thread_local Optional<SharedModelAsset>            model_asset;
    thread_local Optional<SharedJob<SharedModelAsset>> last_model_job;
    thread_local String                                model_vpath;

    thread_local String last_error;

    if (last_texture_job and not last_texture_job->is_ready())
        ImGui::TextUnformatted("Loading Texture...");

    if (last_model_job and not last_model_job->is_ready())
        ImGui::TextUnformatted("Loading Model...");

    if (last_texture_job && last_texture_job->is_ready()) {
        try {
            texture_asset = move_out(last_texture_job).get_result();
            glapi::make_available<Binding::Texture2D>(texture_asset->texture->id());
            last_error = {};
        } catch (const std::exception& e) {
            last_error = e.what();
        }
    }
    if (last_model_job && last_model_job->is_ready()) {
        try {
            model_asset = move_out(last_model_job).get_result();
            for (auto& mesh : model_asset->meshes) {
                visit([&]<typename T>(T& mesh_asset) {
                    if (auto* asset = try_get(mesh_asset.diffuse )) { glapi::make_available<Binding::Texture2D>(asset->texture->id()); }
                    if (auto* asset = try_get(mesh_asset.specular)) { glapi::make_available<Binding::Texture2D>(asset->texture->id()); }
                    if (auto* asset = try_get(mesh_asset.normal  )) { glapi::make_available<Binding::Texture2D>(asset->texture->id()); }
                    glapi::make_available<Binding::ArrayBuffer>       (mesh_asset.vertices->id());
                    glapi::make_available<Binding::ElementArrayBuffer>(mesh_asset.indices->id() );
                }, mesh);
            }
        } catch (const std::exception& e) {
            last_error = e.what();
        }
    }


    if (texture_asset) {
        ImGui::TextUnformatted(texture_asset->path.entry().c_str());
        ImGui::ImageGL(texture_asset->texture->id(), { 480, 480 });
    }
    if (model_asset) {
        ImGui::TextUnformatted(model_asset->path.entry().c_str());
        for (auto& mesh : model_asset->meshes) {
            size_t next_id{ 0 };
            GLuint ids[3];
            visit([&]<typename T>(T& mesh) {
                ImGui::TextUnformatted(mesh.path.subpath().begin());
                if (auto* asset = try_get(mesh.diffuse )) { ids[next_id++] = asset->texture->id(); }
                if (auto* asset = try_get(mesh.specular)) { ids[next_id++] = asset->texture->id(); }
                if (auto* asset = try_get(mesh.normal  )) { ids[next_id++] = asset->texture->id(); }
            }, mesh);
            const auto visible_ids = std::span(ids, next_id);
            using ranges::views::enumerate;
            for (auto [i, id] : enumerate(visible_ids)) {
                ImGui::ImageGL(id, { 64, 64 });
                if (i < visible_ids.size() - 1) { ImGui::SameLine(); }
            }
        }
    }


    auto load_texture_later = on_signal([&] {
        try {
            Path path = vfs().resolve_path(VPath(texture_vpath));
            last_texture_job = asset_manager.load_texture({ MOVE(path) }, ImageIntent::Unknown);
        } catch (const std::exception& e) {
            last_error = e.what();
        }
    });
    auto load_model_later = on_signal([&] {
        try {
            Path path = vfs().resolve_path(VPath(model_vpath));
            last_model_job = asset_manager.load_model({ MOVE(path) });
        } catch (const std::exception& e) {
            last_error = e.what();
        }
    });


    if (ImGui::InputText("VPath##Texture", &texture_vpath, ImGuiInputTextFlags_EnterReturnsTrue)) {
        load_texture_later.set(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Texture")) {
        load_texture_later.set(true);
    }

    if (ImGui::InputText("VPath##Model", &model_vpath, ImGuiInputTextFlags_EnterReturnsTrue)) {
        load_model_later.set(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Model")) {
        load_model_later.set(true);
    }



    if (!last_error.empty()) {
        ImGui::TextUnformatted(last_error.c_str());
    }
}

} // namespace

void ImGuiApplicationAssembly::_draw_widgets()
{
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
    ImGui::DockSpaceOverViewport(dockspace_id, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::PopStyleColor();

    if (not hidden)
    {
        if (ImGui::BeginMainMenuBar())
        {
            ImGui::TextUnformatted("JoshEd");
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

            if (ImGui::BeginMenu("Window"))
            {
                window_settings.display(*this);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("ImGui"))
            {
                ImGui::Checkbox("Render Engine",  &show_engine_hooks   );
                ImGui::Checkbox("Scene",          &show_scene_list     );
                ImGui::Checkbox("Selected",       &show_selected       );
                ImGui::Checkbox("Demo Window",    &show_demo_window    );
                ImGui::Checkbox("Asset Manager",  &show_asset_manager  );
                ImGui::Checkbox("Resource Files", &show_resource_viewer);
                ImGui::Checkbox("Frame Graph",    &show_frame_graph    );
                ImGui::Checkbox("Logs",           &show_log_window     );
                ImGui::Checkbox("Debug",          &show_debug_window   );

                ImGui::Separator();

                ImGui::SliderFloat("FPS Avg. Interval, s", &_avg_frame_timer.averaging_interval,
                    0.001f, 5.f, "%.3f", ImGuiSliderFlags_Logarithmic);

                ImGui::SliderFloat("Bg. Alpha", &background_alpha, 0.f, 1.f);

                ImGui::Checkbox("Gizmo Debug Window", &gizmos.display_debug_window);
                ImGui::EnumListBox("Gizmo Location", &gizmos.preferred_location);

                ImGui::Checkbox("Show Model Matrix in Selected", &selected_menu.display_model_matrix);
                ImGui::Checkbox("Show All Components in Selected", &selected_menu.display_all_components);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Engine"))
            {
                auto& engine = runtime.renderer;
                ImGui::Checkbox("RGB -> sRGB",    &engine.enable_srgb_conversion);
                ImGui::Checkbox("GPU/CPU Timers", &engine.capture_stage_timings );

                ImGui::BeginDisabled(not engine.capture_stage_timings);
                ImGui::SliderFloat("Timing Interval, s", &engine.stage_timing_averaging_interval_s,
                    0.001f, 5.f, "%.3f", ImGuiSliderFlags_Logarithmic);
                ImGui::EndDisabled();

                auto color_format = engine.main_color_format();
                auto depth_format = engine.main_depth_format();
                auto resolution   = engine.main_resolution();

                bool do_respec = false;
                do_respec |= ImGui::EnumCombo("Color Format", &color_format);
                do_respec |= ImGui::EnumCombo("Depth Format", &depth_format);
                ImGui::Checkbox("Fit Window", &engine.fit_window_size);
                ImGui::BeginDisabled(engine.fit_window_size);
                do_respec |= ImGui::SliderInt2("Main Resolution", &resolution.width, 16, 4096);
                ImGui::EndDisabled();
                if (do_respec)
                    engine.respec_main_target(resolution, color_format, depth_format);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("VFS"))
            {
                vfs_control.display(*this);
                ImGui::EndMenu();
            }

            // Logs.
            // TODO: This should probably be removed.
            {
                auto log_view = _log_sink.view();
                const bool new_logs = log_view.size() > _last_log_size;

                if (new_logs)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.0, 1.0, 0.0, 1.0 });

                // NOTE: This is somewhat messy. If this is common, might be worth writing helpers.
                thread_local bool logs_open_b4 = false;
                const bool        logs_open    = ImGui::BeginMenu("Logs");

                if (new_logs)
                    ImGui::PopStyleColor();

                const bool was_closed = not logs_open and logs_open_b4;

                if (logs_open)
                {
                    ImGui::TextUnformatted(log_view.begin(), log_view.end());
                    ImGui::EndMenu();
                }

                if (was_closed)
                    _last_log_size = log_view.size();

                logs_open_b4 = logs_open;
            }

            const auto num_tasks = runtime.async_cradle.task_counter.hint_num_tasks_in_flight();
            if (num_tasks)
                ImGui::Text("[%zu]", num_tasks);

            const float size_gizmo     = ImGui::CalcTextSize(_gizmo_info_str_template).x;
            const float size_fps       = ImGui::CalcTextSize(_fps_str_template).x;
            const float size_frametime = ImGui::CalcTextSize(_frametime_str_template).x;

            ImGui::SameLine(ImGui::GetContentRegionMax().x - (size_gizmo + size_fps + size_frametime));
            ImGui::TextUnformatted(_gizmo_info_str.c_str());
            ImGui::SameLine(ImGui::GetContentRegionMax().x - (size_fps + size_frametime));
            ImGui::TextUnformatted(_fps_str.c_str());
            ImGui::SameLine(ImGui::GetContentRegionMax().x - (size_frametime));
            ImGui::TextUnformatted(_frametime_str.c_str());

            ImGui::EndMainMenuBar();
        }

        if (show_frame_graph)
        {
            if (ImGui::Begin("Frame Graph"))
                _display_frame_graph();
            ImGui::End();
        }

        if (show_engine_hooks)
        {
            if (ImGui::Begin("Render Engine"))
                stage_hooks.display(*this);
            ImGui::End();
        }

        if (show_selected)
        {
            if (ImGui::Begin("Selected"))
                selected_menu.display(*this);
            ImGui::End();
        }

        if (show_scene_list)
        {
            if (ImGui::Begin("Scene"))
                scene_list.display(*this);
            ImGui::End();
        }

        if (show_demo_window)
            ImGui::ShowDemoWindow();

        if (show_asset_manager)
        {
            if (ImGui::Begin("Asset Manager"))
                display_asset_manager_debug(runtime.asset_manager);
            ImGui::End();
        }

        if (show_resource_viewer)
        {
            if (ImGui::Begin("Resources"))
                resource_viewer.display(*this);
            ImGui::End();
        }

        if (show_debug_window)
        {
            if (ImGui::Begin("Debug"))
                _display_debug();
            ImGui::End();
        }

        if (show_log_window)
        {
            if (ImGui::Begin("Logs"))
                ImGui::TextUnformatted(_log_sink.view());
            ImGui::End();
        }
    }
}

void ImGuiApplicationAssembly::display()
{
    _draw_widgets();
    if (const auto camera = get_active<Camera, MTransform>(runtime.registry))
    {
        const mat4 view_mat = inverse(camera.get<MTransform>().model());
        const mat4 proj_mat = camera.get<Camera>().projection_mat();
        gizmos.display(*this, view_mat, proj_mat);
    }
    imgui_context.render();
}

void ImGuiApplicationAssembly::_reset_dockspace(ImGuiID dockspace_id)
{
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

void ImGuiApplicationAssembly::_display_frame_graph()
{
    thread_local bool display_fps = false;

    const char* overlay = nullptr;
    if (display_fps)
    {
        thread_local String overlay_str; overlay_str.clear();
        const float frametime_s = _avg_frame_timer.get_current_average();
        const float fps         = 1 / frametime_s;
        fmt::format_to(std::back_inserter(overlay_str), "{:6>.1f} FPS {:>5.2f} ms", fps, frametime_s * 1e3f);
        overlay = overlay_str.c_str();
    }
    ImGui::PlotLines("##FrameTimes",
        _frame_deltas.data(), int(_frame_deltas.size()), _frame_offset,
        overlay, 0.f, _upper_frametime_limit, ImGui::GetContentRegionAvail());

    ImGui::OpenPopupOnItemClick("FrameGraph Settings");
    if (ImGui::BeginPopup("FrameGraph Settings"))
    {
        ImGui::DragInt("Num Frames", &_num_frames_plotted, 1.f, 1, 1200);
        ImGui::DragFloat("Max Frame Time, ms", &_upper_frametime_limit, 1.f, 0.1f, 200.f);
        ImGui::Checkbox("Display FPS", &display_fps);
        ImGui::EndPopup();
    }
}

void ImGuiApplicationAssembly::_display_debug()
{
    if (ImGui::TreeNode("Texture Swizzle"))
    {
        thread_local SwizzleRGBA swizzle = {};

        ImGui::EnumCombo("R", &swizzle.r);
        ImGui::EnumCombo("G", &swizzle.g);
        ImGui::EnumCombo("B", &swizzle.b);
        ImGui::EnumCombo("A", &swizzle.a);

        if (ImGui::Button("Convert All Diffuse"))
        {
            for (auto [e, mtl] : runtime.registry.view<MaterialDiffuse>().each())
            {
                // NOTE: Effectively doing a GL const_cast.
                RawTexture2D<>::from_id(mtl.texture->id()).set_swizzle_rgba(swizzle);
            }
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Skeletons"))
    {
        DEFER(ImGui::TreePop());

        if (ImGui::TreeNode("Land"))
        {
            DEFER(ImGui::TreePop());

            ImGui::TextUnformatted("Occupied:");
            for (const auto& range : runtime.skeleton_storage._land.view_occupied())
                ImGui::Text("[%zu, %zu]", range.base, range.end());

            ImGui::TextUnformatted("Empty:");
            for (const auto& range : runtime.skeleton_storage._land.view_empty())
                ImGui::Text("[%zu, %zu)", range.base, range.end());
        }

        SkeletonID to_remove = nullid;
        for (const auto& [id, entry] : runtime.skeleton_storage._table)
        {
            ImGui::PushID(id._value); DEFER(ImGui::PopID());
            if (ImGui::Button("x"))
                to_remove = id;
            ImGui::SameLine();
            ImGui::Text("[%zu] %s [%zu, %zu)",
                id._value, entry.name.c_str(), entry.range.base, entry.range.end());
        }

        if (to_remove != nullid)
        {
            runtime.skeleton_storage.remove(to_remove);
        }
    }
}

} // namespace josh
