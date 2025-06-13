#include "ImGuiResourceViewer.hpp"
#include "ECS.hpp"
#include "ImGuiHelpers.hpp"
#include "Common.hpp"
#include "Coroutines.hpp"
#include "DefaultResources.hpp"
#include "Logging.hpp"
#include "ObjectLifecycle.hpp"
#include "Processing.hpp"
#include "Resource.hpp"
#include "ResourceDatabase.hpp"
#include "Throughporters.hpp"
#include "UUID.hpp"
#include <algorithm>
#include <exception>
#include <imgui.h>
#include <imgui_stdlib.h>


namespace josh {


void ImGuiResourceViewer::display_viewer()
{
    thread_local String path;
    thread_local String last_error;

    thread_local Optional<Job<UUID>> importing_job;
    thread_local Optional<UUID>      last_imported;

    thread_local ImportSceneParams   import_scene_params = {};
    thread_local ImportTextureParams import_texture_params = {};

    thread_local Optional<Job<>> unpacking_job;

    thread_local Optional<Job<>> throughporting_job;

    thread_local Optional<inspector_type> current_inspector;


    auto unpack_signal = on_value_change_from<UUID>({}, [&](UUID uuid)
    {
        const Handle handle = create_handle(registry);
        handle.emplace<Transform>();
        try
        {
            unpacking_job = resource_unpacker.unpack_any(uuid, handle);
            last_error = {};
        }
        catch (const std::exception& e)
        {
            last_error = e.what();
        }
    });

    // FIXME: 1-frame delay is ugly.
    auto inspect_signal = on_value_change_from<UUID>({}, [&](UUID uuid)
    {
        try
        {
            const ResourceType type = resource_database.type_of(uuid); // TOCTOU, but rare
            current_inspector = _inspector_factories.at(type)(ResourceInspectorContext(*this), uuid);
            ImGui::OpenPopup("Inspect", ImGuiPopupFlags_NoOpenOverExistingPopup);
        }
        catch (const std::exception& e)
        {
            last_error = e.what();
        }
    });

    // NOTE: Trying to close the unused file.
    // TODO: Is there a better flow for this?
    if (current_inspector and not ImGui::IsPopupOpen("Inspect"))
        current_inspector = nullopt;

    ImGui::Text("Root: %s", resource_database.root().c_str());
    ImGui::InputText("Path", &path);

    ImGui::SameLine();
    if (ImGui::Button("Throughport"))
    {
        try
        {
            const GLTFThroughportParams params = {
                .generate_mips = true,
                .unitarization = Unitarization::InsertDummy,
            };

            const ThroughportContext context = {
                registry,
                mesh_registry,
                skeleton_storage,
                animation_storage,
                async_cradle,
            };

            throughporting_job = throughport_scene_gltf(path, nullent, params, context);

            last_error = {};
        }
        catch (const std::exception& e)
        {
            last_error = e.what();
        }
    }

    auto try_import_thing = [&](auto params)
    {
        try
        {
            importing_job = asset_importer.import_asset(path, params);
            // TODO: Should also log?
            last_error = {};
        }
        catch (const std::exception& e)
        {
            last_error = e.what();
        }
    };

    auto texture_encoding_combo = [&](ImportEncoding& current_format)
    {
        if (ImGui::BeginCombo("Texture Format", enum_cstring(current_format)))
        {
            for (const ImportEncoding enc : enum_iter<ImportEncoding>())
                if (ImGui::Selectable(enum_cstring(enc), current_format == enc))
                    current_format = enc;
            ImGui::EndCombo();
        }
    };

    if (ImGui::TreeNode("Import Texture"))
    {
        texture_encoding_combo(import_texture_params.encoding);
        ImGui::Checkbox("Generate Mipmaps", &import_texture_params.generate_mips);
        if (ImGui::Button("Import")) try_import_thing(import_texture_params);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Import Scene"))
    {
        texture_encoding_combo(import_scene_params.texture_encoding);
        ImGui::Checkbox("Generate Mipmaps", &import_scene_params.generate_mips);
        ImGui::SameLine();
        ImGui::Checkbox("Collapse Graph", &import_scene_params.collapse_graph);
        ImGui::SameLine();
        ImGui::Checkbox("Merge Meshes", &import_scene_params.merge_meshes);
        if (ImGui::Button("Import")) try_import_thing(import_scene_params);
        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Entries"))
    {
        // FIXME: Very crappy filter.
        thread_local ResourceType current_filtered = NullResource;
        thread_local bool         do_filter        = false;

        // TODO: CStrView would be a useful type when dealing with imgui.
        // Right now, we just know that ResourceInfo returns null-terminated strings.
        ImGui::Checkbox("##FilterCheckbox", &do_filter);
        ImGui::SameLine();
        ImGui::BeginDisabled(not do_filter);
        if (ImGui::BeginCombo("Filter##Combo", resource_info().name_or(current_filtered, "None").data())) {
            for (const ResourceType resource_type : resource_info().view_registered())
                if (ImGui::Selectable(resource_info().name_of(resource_type).data(), resource_type == current_filtered))
                    current_filtered = resource_type;
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();

        const auto table_flags =
            ImGuiTableFlags_Borders     |
            ImGuiTableFlags_Resizable   |
            ImGuiTableFlags_Reorderable |
            ImGuiTableFlags_Hideable    |
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_HighlightHoveredColumn;

        if (ImGui::BeginTable("Resources", 5, table_flags))
        {
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("File");
            ImGui::TableSetupColumn("Offset", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableSetupColumn("Size");
            ImGui::TableSetupColumn("UUID", ImGuiTableColumnFlags_DefaultHide);
            ImGui::TableHeadersRow();
            usize i = 0;
            resource_database.for_each_row([&i, &unpack_signal, &inspect_signal](
                const ResourceDatabase::Row& row)
            {
                if (do_filter and not (row.type == current_filtered)) return;
                ImGui::PushID(void_id(i));
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(resource_info().name_or(row.type, "(unknown)"));

                ImGui::TableNextColumn();
                // TODO: We really should guarantee null-termination in the path
                char path[ResourcePath::max_length + 1];
                auto end = std::ranges::copy(row.filepath.view(), path).out;
                *end = '\0';
                // NOTE: No ability to select it yet. Just a visual hover hint.
                ImGui::Selectable(path, false, ImGuiSelectableFlags_SpanAllColumns);
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Unpack"))
                        unpack_signal.set(row.uuid);

                    if (ImGui::MenuItem("Inspect..."))
                        inspect_signal.set(row.uuid);

                    ImGui::EndPopup();
                }

                ImGui::TableNextColumn();
                ImGui::Text("%zu", row.offset_bytes);

                ImGui::TableNextColumn();
                ImGui::Text("%zu", row.size_bytes);

                ImGui::TableNextColumn();
                char uuid_str[37]{};
                serialize_uuid_to(uuid_str, row.uuid);
                ImGui::TextUnformatted(uuid_str);

                ++i;
                ImGui::PopID();
            });

            ImGui::EndTable();
        }
        ImGui::TreePop();
    }

    if (ImGui::BeginPopup("Inspect"))
    {
        // TODO: What if this throws?
        current_inspector.value()();
        ImGui::EndPopup();
    }

    if (importing_job and importing_job->is_ready())
    {
        try
        {
            last_imported = move_out(importing_job).get_result();
            char uuid_string[37]{};
            serialize_uuid_to(uuid_string, *last_imported);
            logstream() << "Imported " << uuid_string << ".\n";
        }
        catch (const std::exception& e)
        {
            logstream() << "Import failed: " << e.what() << "\n";
        }
    }

    if (unpacking_job and unpacking_job->is_ready())
    {
        try
        {
            move_out(unpacking_job).get_result();
            logstream() << "Unpacked ...something.\n";
        }
        catch (const std::exception& e)
        {
            logstream() << "Unpacking failed: " << e.what() << "\n";
        }
    }

    if (throughporting_job and throughporting_job->is_ready())
    {
        try
        {
            move_out(throughporting_job).get_result();
            logstream() << "Throughported... something.\n";
        }
        catch (const std::exception& e)
        {
            logstream() << "Throughporting failed: " << e.what() << "\n";
        }
    }

    ImGui::TextUnformatted(last_error.c_str());
}



} // namespace josh
