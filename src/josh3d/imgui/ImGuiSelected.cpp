#include "ImGuiSelected.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "LightCasters.hpp"
#include "Mesh.hpp"
#include "ObjectLifecycle.hpp"
#include "SceneGraph.hpp"
#include "Skybox.hpp"
#include "TerrainChunk.hpp"
#include "Transform.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/entity.hpp>
#include <imgui.h>


namespace josh {


namespace imgui {


struct GenericHeaderInfo {
    const char* type_name = "UnknownEntityType";
    const char* name      = "";
};


static auto GetGenericHeaderInfo(entt::handle handle)
    -> GenericHeaderInfo
{
    const char* type_name = [&]() {
        if (handle.all_of<Mesh>())             { return "Mesh";             }
        if (handle.all_of<TerrainChunk>())     { return "TerrainChunk";     }
        if (handle.all_of<AmbientLight>())     { return "AmbientLight";     }
        if (handle.all_of<DirectionalLight>()) { return "DirectionalLight"; }
        if (handle.all_of<PointLight>())       { return "PointLight";       }
        if (handle.all_of<Skybox>())           { return "Skybox";           }


        if (handle.all_of<Transform>()) {
            if (has_children(handle)) { return "Node";   }
            else                      { return "Orphan"; }
        } else {
            if (has_children(handle)) { return "GroupingNode";  } // Does this even make sense?
            else                      { return "UnknownEntity"; }
        }
    }();

    const char* name = [&]() {
        if (const Name* name = handle.try_get<Name>()) {
            return name->c_str();
        } else {
            return "";
        }
    }();

    return { type_name, name };
}


void GenericHeaderText(entt::handle handle) {
    const bool is_culled = has_tag<Culled>(handle);
    if (is_culled) {
        auto text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        text_color.w *= 0.5f; // Dim text when culled.
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    }

    auto [type_name, name] = GetGenericHeaderInfo(handle);
    ImGui::Text("[%d] [%s] %s", entt::to_entity(handle.entity()), type_name, name);

    if (is_culled) {
        ImGui::PopStyleColor();
    }
}


} // namespace imgui


void ImGuiSelected::display() {

    // auto to_remove = on_value_change_from<entt::handle>({}, &destroy_subtree);

    for (auto entity : registry_.view<Selected>()) {
        ImGui::PushID(void_id(entity));
        const entt::handle handle{ registry_, entity };


        imgui::GenericHeaderText(handle);

        // Display Transform independent of other components.
        if (auto transform = handle.try_get<Transform>()) {
            imgui::TransformWidget(transform);
        }

        // Mostly for debugging.
        if (display_model_matrix) {
            if (auto mtf = handle.try_get<MTransform>()) {
                imgui::Matrix4x4DisplayWidget(mtf->model());
            }
        }

        if (auto mesh = handle.try_get<Mesh>()) {
            imgui::MaterialsWidget(handle);
        }

        if (auto plight = handle.try_get<PointLight>()) {
            imgui::PointLightWidgetBody(handle);
        }
        if (auto dlight = handle.try_get<DirectionalLight>()) {
            imgui::DirectionalLightWidget(handle);
        }

        if (auto alight = handle.try_get<AmbientLight>()) {
            imgui::AmbientLightWidget(alight);
        }

        ImGui::Separator();
        ImGui::PopID();
    }

}


} // namespace josh
