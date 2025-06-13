#include "ImGuiApplicationAssembly.hpp"
#include "Active.hpp"
#include "Asset.hpp"
#include "AssetManager.hpp"
#include "AsyncCradle.hpp"
#include "CategoryCasts.hpp"
#include "ContainerUtils.hpp"
#include "FrameTimer.hpp"
#include "GLAPIBinding.hpp"
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
#include "Transform.hpp"
#include "VPath.hpp"
#include "VirtualFilesystem.hpp"
#include "glfwpp/window.h"
#include <exception>
#include <fmt/core.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <range/v3/view/enumerate.hpp>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <optional>


namespace josh {


ImGuiApplicationAssembly::ImGuiApplicationAssembly(
    glfw::Window& window,
    Runtime&      runtime
)
    : window(window)
    , runtime(runtime)
    // TODO: DESTROY THIS
    , context(window)
    , window_settings(window)
    , vfs_control(vfs())
    , stage_hooks(runtime.renderer)
    , scene_list(runtime.registry, runtime.asset_manager, runtime.asset_unpacker, runtime.scene_importer)
    , resource_viewer(runtime.resource_database, runtime.asset_importer, runtime.resource_unpacker, runtime.registry, runtime.mesh_registry, runtime.skeleton_storage, runtime.animation_storage, runtime.async_cradle)
    , selected_menu(runtime.registry)
    , gizmos(runtime.registry)
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

    context.new_frame();
    gizmos.new_frame();
}

namespace {

void display_asset_manager_debug(AssetManager& asset_manager)
{
    thread_local std::optional<SharedTextureAsset>            texture_asset;
    thread_local std::optional<SharedJob<SharedTextureAsset>> last_texture_job;
    thread_local std::string                                  texture_vpath;

    thread_local std::optional<SharedModelAsset>            model_asset;
    thread_local std::optional<SharedJob<SharedModelAsset>> last_model_job;
    thread_local std::string                                model_vpath;

    thread_local std::string last_error;


    if (last_texture_job && !last_texture_job->is_ready()) {
        ImGui::TextUnformatted("Loading Texture...");
    }
    if (last_model_job && !last_model_job->is_ready()) {
        ImGui::TextUnformatted("Loading Model...");
    }


    if (last_texture_job && last_texture_job->is_ready()) {
        try {
            texture_asset = move_out(last_texture_job).get_result();
            make_available<Binding::Texture2D>(texture_asset->texture->id());
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
                    if (auto* asset = try_get(mesh_asset.diffuse )) { make_available<Binding::Texture2D>(asset->texture->id()); }
                    if (auto* asset = try_get(mesh_asset.specular)) { make_available<Binding::Texture2D>(asset->texture->id()); }
                    if (auto* asset = try_get(mesh_asset.normal  )) { make_available<Binding::Texture2D>(asset->texture->id()); }
                    make_available<Binding::ArrayBuffer>       (mesh_asset.vertices->id());
                    make_available<Binding::ElementArrayBuffer>(mesh_asset.indices->id() );
                }, mesh);
            }
        } catch (const std::exception& e) {
            last_error = e.what();
        }
    }


    if (texture_asset) {
        ImGui::TextUnformatted(texture_asset->path.entry().c_str());
        imgui::ImageGL(texture_asset->texture->id(), { 480, 480 });
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
                imgui::ImageGL(id, { 64, 64 });
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

    if (not hidden)
    {
        auto reset_later = on_signal([&, this] { reset_dockspace(dockspace_id); });

        auto bg_col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
        bg_col.w = background_alpha;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_col);

        if (ImGui::BeginMainMenuBar())
        {
            ImGui::TextUnformatted("Josh3D-Demo");
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

            if (ImGui::BeginMenu("Window"))
            {
                window_settings.display();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("ImGui"))
            {
                ImGui::Checkbox("Render Engine",  &show_engine_hooks  );
                ImGui::Checkbox("Scene",          &show_scene_list    );
                ImGui::Checkbox("Selected",       &show_selected      );
                ImGui::Checkbox("Demo Window",    &show_demo_window   );
                ImGui::Checkbox("Asset Manager",  &show_asset_manager );
                ImGui::Checkbox("Resource Files", &show_resource_viewer);
                ImGui::Checkbox("Frame Graph",    &show_frame_graph   );
                ImGui::Checkbox("Debug",          &show_debug_window  );

                ImGui::Separator();

                ImGui::SliderFloat("FPS Avg. Interval, s", &_avg_frame_timer.averaging_interval,
                    0.001f, 5.f, "%.3f", ImGuiSliderFlags_Logarithmic);

                ImGui::SliderFloat("Bg. Alpha", &background_alpha, 0.f, 1.f);

                reset_later.set(ImGui::Button("Reset Dockspace"));

                ImGui::Checkbox("Gizmo Debug Window", &gizmos.display_debug_window);

                const char* gizmo_locations[] = {
                    "Local Origin",
                    "AABB Midpoint"
                };

                // TODO: EnumListBox
                int location_id = to_underlying(gizmos.preferred_location);
                if (ImGui::ListBox("Gizmo Locaton", &location_id,
                    gizmo_locations, std::size(gizmo_locations), 2))
                {
                    gizmos.preferred_location = GizmoLocation(location_id);
                }

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

                // TODO: EnumCombo
                if (ImGui::BeginCombo("HDR Format", enum_cstring(engine.main_buffer_format)))
                {
                    for (const auto format : enum_iter<HDRFormat>())
                        if (ImGui::Selectable(enum_cstring(format), format == engine.main_buffer_format))
                            engine.main_buffer_format = format;
                    ImGui::EndCombo();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("VFS"))
            {
                vfs_control.display();
                ImGui::EndMenu();
            }

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
                display_frame_graph();
            ImGui::End();
        }

        if (show_engine_hooks)
        {
            if (ImGui::Begin("Render Engine"))
                stage_hooks.display();
            ImGui::End();
        }

        if (show_selected)
        {
            if (ImGui::Begin("Selected"))
                selected_menu.display();
            ImGui::End();
        }

        if (show_scene_list)
        {
            if (ImGui::Begin("Scene"))
                scene_list.display();
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
                resource_viewer.display_viewer();
            ImGui::End();
        }

        if (show_debug_window)
        {
            if (ImGui::Begin("Debug"))
                display_debug();
            ImGui::End();
        }

        ImGui::PopStyleColor();
    }
}

void ImGuiApplicationAssembly::display()
{
    draw_widgets();
    if (const auto camera = get_active<Camera, MTransform>(runtime.registry))
    {
        const mat4 view_mat = inverse(camera.get<MTransform>().model());
        const mat4 proj_mat = camera.get<Camera>().projection_mat();
        gizmos.display(view_mat, proj_mat);
    }
    context.render();
}

void ImGuiApplicationAssembly::reset_dockspace(ImGuiID dockspace_id)
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

void ImGuiApplicationAssembly::display_frame_graph()
{
    ImGui::PlotLines("##FrameTimes",
        _frame_deltas.data(), int(_frame_deltas.size()), _frame_offset,
        nullptr, 0.f, _upper_frametime_limit, ImGui::GetContentRegionAvail());

    ImGui::OpenPopupOnItemClick("FrameGraph Settings");
    if (ImGui::BeginPopup("FrameGraph Settings"))
    {
        ImGui::DragInt("Num Frames", &_num_frames_plotted, 1.f, 1, 1200);
        ImGui::DragFloat("Max Frame Time, ms", &_upper_frametime_limit, 1.f, 0.1f, 200.f);
        ImGui::EndPopup();
    }
}

void ImGuiApplicationAssembly::display_debug()
{
    if (ImGui::TreeNode("Texture Swizzle"))
    {
        thread_local SwizzleRGBA swizzle = {};

        auto selector = [](const char* channel, Swizzle* tgt)
        {
            if (ImGui::BeginCombo(channel, enum_cstring(*tgt)))
            {
                for (const Swizzle s : enum_iter<Swizzle>())
                    if (ImGui::Selectable(enum_cstring(s), *tgt == s))
                        *tgt = s;
                ImGui::EndCombo();
            }
        };

        selector("R", &swizzle.r);
        selector("G", &swizzle.g);
        selector("B", &swizzle.b);
        selector("A", &swizzle.a);

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
}

} // namespace josh
