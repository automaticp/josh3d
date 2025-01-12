#include "ImGuiApplicationAssembly.hpp"
#include "Active.hpp"
#include "Asset.hpp"
#include "AssetManager.hpp"
#include "CategoryCasts.hpp"
#include "ContainerUtils.hpp"
#include "FrameTimer.hpp"
#include "GLAPIBinding.hpp"
#include "GLBuffers.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "ImGuiHelpers.hpp"
#include "ImGuiSceneList.hpp"
#include "ImGuiSelected.hpp"
#include "ImGuizmoGizmos.hpp"
#include "Camera.hpp"
#include "MeshRegistry.hpp"
#include "RenderEngine.hpp"
#include "ResourceFiles.hpp"
#include "SceneImporter.hpp"
#include "Region.hpp"
#include "SkinnedMesh.hpp"
#include "Transform.hpp"
#include "VPath.hpp"
#include "VertexPNUTB.hpp"
#include "VirtualFilesystem.hpp"
#include <exception>
#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/view/enumerate.hpp>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <optional>




namespace josh {


ImGuiApplicationAssembly::ImGuiApplicationAssembly(
    glfw::Window&      window,
    RenderEngine&      engine,
    entt::registry&    registry,
    AssetManager&      asset_manager,
    AssetUnpacker&     asset_unpacker,
    SceneImporter&     scene_importer,
    VirtualFilesystem& vfs
)
    : window_         { window             }
    , engine_         { engine             }
    , registry_       { registry           }
    , asset_manager_  { asset_manager      }
    , asset_unpacker_ { asset_unpacker     }
    , vfs_            { vfs                }
    , context_        { window             }
    , window_settings_{ window             }
    , vfs_control_    { vfs                }
    , stage_hooks_    { engine             }
    , scene_list_     { registry, asset_manager, asset_unpacker, scene_importer }
    , asset_browser_  { asset_manager      }
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



namespace {


void display_asset_manager_debug(AssetManager& asset_manager) {
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


void display_resource_file_debug(
    const Registry&     registry,
    const MeshRegistry& mesh_registry)
{
    thread_local std::string path;
    thread_local std::string last_error;
    thread_local size_t      selected_id{};
    thread_local std::vector<Entity> id2entity;
    thread_local std::optional<SkeletonFile> opened_file;

    ImGui::InputText("Path", &path);


    ImGui::SeparatorText("SkeletonFile");
    ImGui::PushID("SkeletonFile");


    if (ImGui::TreeNode("Save")) {

        if (ImGui::BeginListBox("Entities")) {
            id2entity.clear();
            size_t id{};
            char name[16]{ '\0' };
            for (const auto [e, mesh] : registry.view<SkinnedMesh>().each()) {
                std::snprintf(name, std::size(name), "%d", to_entity(e));
                if (ImGui::Selectable(name, id == selected_id)) {
                    selected_id = id;
                }
                id2entity.emplace_back(e);
                ++id;
            }
            ImGui::EndListBox();
        }

        if (ImGui::Button("Save")) {
            try {
                const Entity entity = id2entity.at(selected_id);
                const Skeleton& skeleton = *registry.get<SkinnedMesh>(entity).pose.skeleton;
                auto file = SkeletonFile::create(path, skeleton.joints.size());
                std::span<Joint> file_joints = file.joints();
                assert(file_joints.size() == skeleton.joints.size());
                std::ranges::copy(skeleton.joints, file_joints.begin());
                last_error = {};
            } catch (const std::exception& e) {
                last_error = e.what();
            }
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Open")) {

        if (ImGui::Button("Open")) {
            try {
                opened_file = SkeletonFile::open(path);
                last_error = {};
            } catch (const std::exception& e) {
                last_error = e.what();
            }
        }

        if (auto* f = try_get(opened_file)) {
            ImGui::Text("Num Joints: %d", f->num_joints());
            using ranges::views::enumerate;
            for (const auto [j, joint] : enumerate(f->joints())) {
                ImGui::Text("Joint ID: %d, Parent ID: %d", int(j), joint.parent_id);
                imgui::Matrix4x4DisplayWidget(joint.inv_bind);
            }
        }


        ImGui::TreePop();
    }


    ImGui::PopID();


    ImGui::SeparatorText("MeshFile");
    ImGui::PushID("MeshFile");


    if (ImGui::TreeNode("Save")) {

        if (ImGui::BeginListBox("Entities")) {
            id2entity.clear();
            size_t id{};
            char name[16]{};
            auto iterate_view = [&id, &name](auto&& view, const char* type) {
                for (const Entity e : view) {
                    std::snprintf(name, std::size(name), "%s %d", type, to_entity(e));
                    if (ImGui::Selectable(name, id == selected_id)) {
                        selected_id = id;
                    }
                    id2entity.emplace_back(e);
                    ++id;
                }
            };
            iterate_view(registry.view<MeshID<VertexPNUTB>>(), "Static" );
            iterate_view(registry.view<SkinnedMesh>(),         "Skinned");
            ImGui::EndListBox();
        }

        if (ImGui::Button("Save")) {
            try {
                const Entity entity  = id2entity.at(selected_id);
                using VertexT = VertexPNUTB;
                auto download_from_storage = [&]<typename VertexT>(MeshID<VertexT> mesh_id) {
                    const auto&  storage = *mesh_registry.storage_for<VertexT>();
                    auto [vert_range, elem_range] = storage.buffer_ranges(mesh_id);
                    using LODSpec = MeshFile::LODSpec;
                    constexpr auto layout = MeshFile::vertex_traits<VertexT>::layout;
                    LODSpec specs[1]{
                        LODSpec{
                            .num_verts = uint32_t(vert_range.count),
                            .num_elems = uint32_t(elem_range.count)
                        },
                    };
                    auto file = MeshFile::create(path, layout, specs);
                    auto dst_verts = file.template lod_verts<layout>(0);
                    auto dst_elems = file.lod_elems(0);
                    storage.vertex_buffer().download_data_into(dst_verts, vert_range.offset);
                    storage.index_buffer() .download_data_into(dst_elems, elem_range.offset);
                };

                if (const auto* static_mesh = registry.try_get<MeshID<VertexT>>(entity)) {
                    download_from_storage(*static_mesh);
                } else if (const auto* skinned_mesh = registry.try_get<SkinnedMesh>(entity)) {
                    download_from_storage(skinned_mesh->mesh_id);
                } else { assert(false); }

                last_error = {};
            } catch (const std::exception& e) {
                last_error = e.what();
            }
        }


        ImGui::TreePop();
    }


    ImGui::PopID();


    ImGui::TextUnformatted(last_error.c_str());
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

    if (!is_hidden()) {

        auto reset_later = on_signal([&, this] { reset_dockspace(dockspace_id); });

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
                ImGui::Checkbox("Render Engine",  &show_engine_hooks  );
                ImGui::Checkbox("Scene",          &show_scene_list    );
                ImGui::Checkbox("Selected",       &show_selected      );
                ImGui::Checkbox("Assets",         &show_asset_browser );
                ImGui::Checkbox("Demo Window",    &show_demo_window   );
                ImGui::Checkbox("Asset Manager",  &show_asset_manager );
                ImGui::Checkbox("Resource Files", &show_resource_files);

                ImGui::Separator();

                ImGui::SliderFloat(
                    "FPS Avg. Interval, s", &avg_frame_timer_.averaging_interval,
                    0.001f, 5.f, "%.3f", ImGuiSliderFlags_Logarithmic
                );

                ImGui::SliderFloat("Bg. Alpha", &background_alpha, 0.f, 1.f);

                reset_later.set(ImGui::Button("Reset Dockspace"));

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


        if (show_engine_hooks) {
            if (ImGui::Begin("Render Engine")) {
                stage_hooks_.display();
            } ImGui::End();
        }

        if (show_selected) {
            if (ImGui::Begin("Selected")) {
                selected_menu_.display();
            } ImGui::End();
        }

        if (show_scene_list) {
            if (ImGui::Begin("Scene")) {
                scene_list_.display();
            } ImGui::End();
        }

        if (show_asset_browser) {
            if (ImGui::Begin("Assets")) {
                asset_browser_.display();
            } ImGui::End();
        }

        if (show_demo_window) {
            ImGui::ShowDemoWindow();
        }

        if (show_asset_manager) {
            if (ImGui::Begin("Asset Manager")) {
                display_asset_manager_debug(asset_manager_);
            } ImGui::End();
        }

        if (show_resource_files) {
            if (ImGui::Begin("Resource Files")) {
                display_resource_file_debug(registry_, engine_.mesh_registry());
            } ImGui::End();
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
