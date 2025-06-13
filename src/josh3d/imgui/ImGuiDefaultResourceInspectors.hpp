#pragma once
#include "ContainerUtils.hpp"
#include "DefaultResourceFiles.hpp"
#include "DefaultResources.hpp"
#include "FileMapping.hpp"
#include "ImGuiHelpers.hpp"
#include "ImGuiResourceViewer.hpp"
#include "Ranges.hpp"
#include "UUID.hpp"
#include <imgui.h>
#include <jsoncons/basic_json.hpp>


namespace josh {


struct TextureInspector
{
    ResourceInspectorContext context;
    UUID                     uuid;

    TextureFile file = TextureFile::open(context.resource_database().map_resource(uuid));

    void operator()()
    {
        const auto& header = file.header();
        const auto table_flags =
            ImGuiTableFlags_Borders           |
            ImGuiTableFlags_Resizable         |
            ImGuiTableFlags_Reorderable       |
            ImGuiTableFlags_Hideable          |
            ImGuiTableFlags_SizingStretchProp |
            ImGuiTableFlags_HighlightHoveredColumn;

        if (ImGui::BeginTable("MIPs", 4, table_flags))
        {
            ImGui::TableSetupColumn("Level");
            ImGui::TableSetupColumn("Resolution");
            ImGui::TableSetupColumn("Encoding");
            ImGui::TableSetupColumn("Size");
            ImGui::TableHeadersRow();

            for (const auto mip_id : irange(header.num_mips))
            {
                const auto& mip = file.mip_span(mip_id);
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%zu", mip_id);

                ImGui::TableNextColumn();
                const auto [w, h] = Extent2I(mip.width, mip.height);
                ImGui::Text("%dx%d", w, h);

                ImGui::TableNextColumn();
                const auto encoding = mip.encoding;
                ImGui::Text("%s", enum_cstring(encoding));

                ImGui::TableNextColumn();
                const size_t size = mip.size_bytes;
                ImGui::Text("%zu", size);
            }

            ImGui::EndTable();
        }

    }
};


struct SceneInspector
{
    ResourceInspectorContext context;
    UUID                     uuid;

    jsoncons::json file = eval%[this] {
        const auto mregion = context.resource_database().map_resource(uuid);
        const auto text    = to_span<char>(mregion);
        const auto view    = StrView(text.begin(), text.end());
        return jsoncons::json::parse(view);
    };

    void operator()()
    {
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

            if (ImGui::BeginTable("Entries", 3, table_flags))
            {
                ImGui::TableSetupColumn("Index");
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("UUID");
                ImGui::TableHeadersRow();

                for (auto [i, entry] : enumerate(entries_range))
                {
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
