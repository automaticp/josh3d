#include "ImGuiSceneList.hpp"
#include "ImGuiHelpers.hpp"
#include "LightCasters.hpp"
#include "Mesh.hpp"
#include "Name.hpp"
#include "SceneGraph.hpp"
#include "Skybox.hpp"
#include "Tags.hpp"
#include "tags/Culled.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <imgui.h>


namespace josh {
namespace {


struct SelectionSignal {
    entt::entity target_entity = entt::null;
    bool         toggle_mode   = false;
};


bool begin_entity_display(entt::handle handle, SelectionSignal& selection) {
    const char* type = [&]() {
        if (handle.all_of<Mesh>())             { return "Mesh";             }
        if (handle.all_of<AmbientLight>())     { return "AmbientLight";     }
        if (handle.all_of<DirectionalLight>()) { return "DirectionalLight"; }
        if (handle.all_of<PointLight>())       { return "PointLight";       }
        if (handle.all_of<Skybox>())           { return "Skybox";           }

        if (has_children(handle)) { return "Node"; }

        return "Orphan";
    }();

    const char* name = [&]() {
        if (const Name* name = handle.try_get<Name>()) {
            return name->c_str();
        } else {
            return "";
        }
    }();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (has_tag<Selected>(handle)) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (!has_children(handle)) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    }

    const bool is_culled = has_tag<Culled>(handle);
    if (is_culled) {
        auto text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        text_color.w *= 0.5f; // Dim text when culled.
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    }

    const bool is_node_open = ImGui::TreeNodeEx(
        void_id(handle.entity()), flags,
        "[%d] [%s] %s", entt::to_entity(handle.entity()), type, name
    );

    if (is_culled) {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selection.target_entity = handle.entity();
        selection.toggle_mode   = ImGui::GetIO().KeyCtrl;
    }


    return is_node_open;
}


void end_entity_display() {
    ImGui::TreePop();
}


void display_node_recursive(entt::handle handle, SelectionSignal& selection) {
    if (begin_entity_display(handle, selection)) {
        if (has_children(handle)) {
            for (const entt::handle child_handle : view_child_handles(handle)) {
                display_node_recursive(child_handle, selection);
            }
        }
        end_entity_display();
    }
}


} // namespace




void ImGuiSceneList::display() {
    auto& registry = registry_;

    SelectionSignal selection{}; // To handle selection ouside of the loop.


    for (entt::entity entity :
        registry.view<entt::entity>(entt::exclude<AsChild>))
    {
        const entt::handle handle{ registry, entity };
        display_node_recursive(handle, selection);
    }


    if (selection.target_entity != entt::null) {
        const entt::handle target_handle{ registry, selection.target_entity };

        if (selection.toggle_mode) {
            switch_tag<Selected>(target_handle);
        } else {
            registry.clear<Selected>();
            set_tag<Selected>(target_handle);
        }
    }
}


} // namespace josh
