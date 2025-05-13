#pragma once
#include "DefaultResources.hpp"
#include "FileMapping.hpp"
#include "ImGuiHelpers.hpp"
#include "ImGuiResourceViewer.hpp"
#include "Ranges.hpp"
#include "ResourceFiles.hpp"
#include "UUID.hpp"
#include <imgui.h>
#include <jsoncons/basic_json.hpp>


namespace josh {


struct TextureInspector {
    ResourceInspectorContext context;
    UUID                     uuid;

    TextureFile file = TextureFile::open(context.resource_database().map_resource(uuid));

    void operator()() {
        const auto table_flags =
            ImGuiTableFlags_Borders           |
            ImGuiTableFlags_Resizable         |
            ImGuiTableFlags_Reorderable       |
            ImGuiTableFlags_Hideable          |
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_HighlightHoveredColumn;

        if (ImGui::BeginTable("MIPs", 4, table_flags)) {
            ImGui::TableSetupColumn("Level");
            ImGui::TableSetupColumn("Resolution");
            ImGui::TableSetupColumn("Format");
            ImGui::TableSetupColumn("Size");
            ImGui::TableHeadersRow();
            for (const auto mip_id : irange(file.num_mips())) {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%zu", mip_id);

                ImGui::TableNextColumn();
                const auto [w, h] = file.resolution(mip_id);
                ImGui::Text("%dx%d", w, h);

                ImGui::TableNextColumn();
                const auto format = file.format(mip_id);
                ImGui::Text("%s", enum_cstring(format));

                ImGui::TableNextColumn();
                const size_t size = file.mip_size_bytes(mip_id);
                ImGui::Text("%zu", size);
            }

            ImGui::EndTable();
        }

    }
};


struct SceneInspector {
    ResourceInspectorContext context;
    UUID                     uuid;

    jsoncons::json file = [this]{
        const auto mregion = context.resource_database().map_resource(uuid);
        const auto text    = to_span<char>(mregion);
        const auto view    = StrView(text.begin(), text.end());
        return jsoncons::json::parse(view);
    }();

    void operator()() {
        if (auto& entries = file.at_or_null("entities");
            not entries.is_null())
        {
            if (not entries.is_array()) return;
            const auto entries_range = entries.array_range();
            const auto table_flags =
                ImGuiTableFlags_Borders           |
                ImGuiTableFlags_Resizable         |
                ImGuiTableFlags_Reorderable       |
                ImGuiTableFlags_Hideable          |
                ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_HighlightHoveredColumn;

            if (ImGui::BeginTable("Entries", 3, table_flags)) {
                ImGui::TableSetupColumn("Index");
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("UUID");
                ImGui::TableHeadersRow();
                for (auto [i, entry] : enumerate(entries_range)) {
                    ImGui::PushID(void_id(i));
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%zu", i);

                    ImGui::TableNextColumn();
                    auto& name = entry.at_or_null("name");
                    if (not name.is_null()) {
                        ImGui::TextUnformatted(name.as_string_view());
                    }

                    ImGui::TableNextColumn();
                    auto& uuid = entry.at_or_null("uuid");
                    if (not uuid.is_null()) {
                        ImGui::TextUnformatted(uuid.as_string_view());
                    }

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
    }
};


inline void register_default_resource_inspectors(
    ImGuiResourceViewer& v)
{
    v.register_inspector<TextureInspector>(RT::Texture);
    v.register_inspector<SceneInspector>(RT::Scene);
}



} // namespace josh
