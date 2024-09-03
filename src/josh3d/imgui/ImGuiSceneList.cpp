#include "ImGuiSceneList.hpp"
#include "Filesystem.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "ImGuiHelpers.hpp"
#include "ObjectLifecycle.hpp"
#include "SceneGraph.hpp"
#include "Tags.hpp"
#include "tags/Culled.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <exception>
#include <imgui.h>
#include <imgui_stdlib.h>


namespace josh {
namespace {


struct Signals {

    struct Selection {
        entt::entity target      = entt::null;
        bool         toggle_mode = false;
    } selection;

    struct DetachFromParent {
        entt::entity target = entt::null;
    } detach_from_parent;

    struct AttachSelected {
        entt::entity target = entt::null;
    } attach_selected;

    struct Destroy {
        entt::entity target           = entt::null;
        bool         with_descendants = false;
    } destroy;

};


void display_item_popup(
    entt::handle handle,
    Signals&     signals)
{
    imgui::GenericHeaderText(handle);


    ImGui::Separator();

    if (ImGui::MenuItem("Select")) {
        signals.selection.target      = handle.entity();
        signals.selection.toggle_mode = false;
    }

    if (ImGui::MenuItem("Select (Toggle)")) {
        signals.selection.target      = handle.entity();
        signals.selection.toggle_mode = true;
    }


    ImGui::Separator();

    if (ImGui::MenuItem("Attach Selected")) {
        signals.attach_selected.target = handle.entity();
    }

    ImGui::BeginDisabled(!has_parent(handle));
    if (ImGui::MenuItem("Detach from Parent")) {
        signals.detach_from_parent.target = handle.entity();
    }
    ImGui::EndDisabled();



    ImGui::Separator();

    if (ImGui::MenuItem("Destroy")) {
        signals.destroy.target           = handle.entity();
        signals.destroy.with_descendants = false;
    }

    ImGui::BeginDisabled(!has_children(handle));
    if (ImGui::MenuItem("Destroy Subtree")) {
        signals.destroy.target           = handle.entity();
        signals.destroy.with_descendants = true;
    }
    ImGui::EndDisabled();

}


bool begin_entity_display(
    entt::handle handle,
    Signals&     signals)
{
    auto [type_name, name] = imgui::GetGenericHeaderInfo(handle);

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
        "[%d] [%s] %s", entt::to_entity(handle.entity()), type_name, name
    );

    if (is_culled) {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        signals.selection.target      = handle.entity();
        signals.selection.toggle_mode = ImGui::GetIO().KeyCtrl;
    }

    if (ImGui::BeginPopupContextItem()) {
        display_item_popup(handle, signals);
        ImGui::EndPopup();
    }

    return is_node_open;
}


void end_entity_display() {
    ImGui::TreePop();
}


void display_node_recursive(
    entt::handle handle,
    Signals&     signals)
{
    if (begin_entity_display(handle, signals)) {
        if (has_children(handle)) {
            for (const entt::handle child_handle : view_child_handles(handle)) {
                display_node_recursive(child_handle, signals);
            }
        }
        end_entity_display();
    }
}


} // namespace




void ImGuiSceneList::display() {
    auto& registry = registry_;

    Signals signals{}; // To handle selection, scene graph modification and destruction outside the loop.


    for (entt::entity entity :
        registry.view<entt::entity>(entt::exclude<AsChild>))
    {
        const entt::handle handle{ registry, entity };
        display_node_recursive(handle, signals);
    }


    thread_local std::string import_model_vpath;
    thread_local AssetPath   import_model_apath;
    thread_local std::string import_model_error_message;

    thread_local std::string import_skybox_vpath;
    thread_local AssetPath   import_skybox_apath;
    thread_local std::string import_skybox_error_message;

    bool open_import_model_popup  = false; // Just opens the popup.
    bool open_import_skybox_popup = false;

    bool import_model_signal      = false; // Actually sends the load request.
    bool import_skybox_signal     = false;



    if (ImGui::BeginPopupContextWindow(nullptr,
        ImGuiPopupFlags_MouseButtonRight |
        ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::BeginMenu("New")) {
            // TODO:
            ImGui::MenuItem("Node");
            ImGui::MenuItem("PointLight");
            ImGui::MenuItem("TerrainChunk");
            ImGui::MenuItem("Actually not implemented...");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Import")) {
            open_import_model_popup  = ImGui::MenuItem("Model");
            open_import_skybox_popup = ImGui::MenuItem("Skybox");

            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }




    if (open_import_model_popup) {
        ImGui::OpenPopup("ImportModelPopup");
    }
    if (ImGui::BeginPopup("ImportModelPopup")) {
        bool should_load = false;

        if (open_import_model_popup) {
            ImGui::SetKeyboardFocusHere(); // On first open, focus on text input.
        }
        const auto text_flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
        should_load |= ImGui::InputText("Model VPath", &import_model_vpath, text_flags);
        should_load |= ImGui::Button("Load");

        if (!import_model_error_message.empty()) {
            ImGui::SameLine();
            ImGui::TextUnformatted(import_model_error_message.c_str());
        }

        if (should_load) {
            // Try resolving the VPath here, if that fails, display the error immediately.
            try {
                import_model_apath         = { File(VPath(import_model_vpath)), {} };
                import_model_signal        = true;
                import_model_error_message = {};
                ImGui::CloseCurrentPopup();
            } catch (const std::exception& e) {
                import_model_error_message = e.what();
            }
        }

        ImGui::EndPopup();
    }


    if (open_import_skybox_popup) {
        ImGui::OpenPopup("ImportSkyboxPopup");
    }
    if (ImGui::BeginPopup("ImportSkyboxPopup")) {
        bool should_load = false;

        if (open_import_skybox_popup) {
            ImGui::SetKeyboardFocusHere(); // On first open, focus on text input.
        }
        const auto text_flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
        should_load |= ImGui::InputText("Skybox JSON VPath", &import_skybox_vpath, text_flags);
        should_load |= ImGui::Button("Load");

        if (!import_skybox_error_message.empty()) {
            ImGui::SameLine();
            ImGui::TextUnformatted(import_skybox_error_message.c_str());
        }

        if (should_load) {
            try {
                import_skybox_apath         = { File(VPath(import_skybox_vpath)), {} };
                import_skybox_signal        = true;
                import_skybox_error_message = {};
                ImGui::CloseCurrentPopup();
            } catch (const std::exception& e) {
                import_skybox_error_message = e.what();
            }
        }

        ImGui::EndPopup();
    }



    // Handle signals.

    if (signals.selection.target != entt::null) {
        const entt::handle target_handle{ registry, signals.selection.target };

        if (signals.selection.toggle_mode) {
            switch_tag<Selected>(target_handle);
        } else {
            registry.clear<Selected>();
            set_tag<Selected>(target_handle);
        }
    }


    if (signals.detach_from_parent.target != entt::null) {
        const entt::handle target_handle{ registry, signals.detach_from_parent.target };
        if (has_parent(target_handle)) {
            detach_from_parent(target_handle);
        }
    }


    if (signals.attach_selected.target != entt::null) {
        const entt::handle target_handle{ registry, signals.attach_selected.target };

        const bool was_selected = unset_tag<Selected>(target_handle); // To not detach the target itself.

        for (const entt::entity entity : registry.view<Selected>()) {
            const entt::handle handle{ registry, entity };
            if (has_parent(handle)) {
                detach_from_parent(handle);
            }
        }

        attach_children(target_handle, registry.view<Selected>());

        if (was_selected) {
            set_tag<Selected>(target_handle); // Restore selected state of the target.
        }
    }


    if (signals.destroy.target != entt::null) {
        const entt::handle target_handle{ registry, signals.destroy.target };

        if (signals.destroy.with_descendants) {
            destroy_subtree(target_handle);
        } else {
            destroy_and_orphan_children(target_handle);
        }
    }


    if (import_model_signal) {
        importer_.request_model_import(import_model_apath);
    }


    if (import_skybox_signal) {
        importer_.request_skybox_import(import_skybox_apath);
    }
}


} // namespace josh
