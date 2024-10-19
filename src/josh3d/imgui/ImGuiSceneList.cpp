#include "ImGuiSceneList.hpp"
#include "Active.hpp"
#include "Camera.hpp"
#include "Filesystem.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "ImGuiHelpers.hpp"
#include "LightCasters.hpp"
#include "ObjectLifecycle.hpp"
#include "SceneGraph.hpp"
#include "Tags.hpp"
#include "TerrainChunk.hpp"
#include "Transform.hpp"
#include "tags/Selected.hpp"
#include "tags/ShadowCasting.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <exception>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>


namespace josh {
namespace {


struct Signals {

    struct MakeActive {
        entt::entity target = entt::null;
    } make_active;

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
    const auto [can_be_active, is_active] = imgui::GetGenericActiveInfo(handle);

    ImGui::Separator();

    ImGui::BeginDisabled(!can_be_active || is_active);
    if (ImGui::MenuItem("Make Active")) {
        signals.make_active.target = handle.entity();
    }
    ImGui::EndDisabled();

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

    const bool is_visible = imgui::GetGenericVisibility(handle);
    if (!is_visible) {
        auto text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        text_color.w *= 0.5f; // Dim text when culled.
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    }

    const bool is_node_open = ImGui::TreeNodeEx(
        void_id(handle.entity()), flags,
        "[%d] [%s] %s", entt::to_entity(handle.entity()), type_name, name
    );

    if (!is_visible) {
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

    thread_local glm::vec3 new_node_position{ 0.f, 1.f, 0.f };

    bool create_new_node = false;

    thread_local PointLight new_plight_template{ .color{ 1.f, 1.f, 1.f }, .power = 10.f };
    thread_local glm::vec3  new_plight_position{ 0.f, 1.f, 0.f };
    thread_local bool       new_plight_cast_shadow{ true };

    bool create_new_plight = false;

    thread_local DirectionalLight new_dlight_template{ .color{ 1.f, 1.f, 1.f } };
    thread_local bool             new_dlight_cast_shadow{ true };

    bool create_new_dlight = false;


    thread_local AmbientLight new_alight_template{ .color{ 0.1f, 0.1f, 0.1f } };

    bool create_new_alight = false;

    thread_local float      new_terrain_max_height = 5.f;
    thread_local glm::vec2  new_terrain_extents   { 100.f, 100.f };
    thread_local glm::ivec2 new_terrain_resolution{ 100,   100   };

    bool create_new_terrain = false;

    thread_local glm::vec3 new_camera_position{ 0.f, 1.f, 0.f };

    bool create_new_camera = false;


    thread_local std::string import_model_vpath;
    thread_local AssetPath   import_model_apath;
    thread_local std::string import_model_error_message;

    bool open_import_model_popup  = false; // Just opens the popup.
    bool import_model_signal      = false; // Actually sends the load request.

    thread_local std::string import_skybox_vpath;
    thread_local AssetPath   import_skybox_apath;
    thread_local std::string import_skybox_error_message;

    bool open_import_skybox_popup = false;
    bool import_skybox_signal     = false;



    if (ImGui::BeginPopupContextWindow(nullptr,
        ImGuiPopupFlags_MouseButtonRight |
        ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::BeginMenu("New")) {

            if (ImGui::BeginMenu("Node")) {
                if (ImGui::IsItemClicked()) {
                    create_new_node = true;
                }
                ImGui::DragFloat3("Position", value_ptr(new_plight_position), 0.2f, -FLT_MAX, FLT_MAX);
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("PointLight")) {
                if (ImGui::IsItemClicked()) {
                    create_new_plight = true;
                }
                ImGui::DragFloat3("Position", value_ptr(new_plight_position), 0.2f, -FLT_MAX, FLT_MAX);
                imgui::PointLightWidget(new_plight_template);
                ImGui::Checkbox("Shadow", &new_plight_cast_shadow);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("AmbientLight")) {
                if (ImGui::IsItemClicked()) {
                    create_new_alight = true;
                }
                imgui::AmbientLightWidget(new_alight_template);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("DirectionalLight")) {
                if (ImGui::IsItemClicked()) {
                    create_new_dlight = true;
                }
                imgui::DirectionalLightWidget(new_dlight_template);
                ImGui::Checkbox("Shadow", &new_dlight_cast_shadow);
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("TerrainChunk")) {
                if (ImGui::IsItemClicked()) {
                    create_new_terrain = true;
                }
                ImGui::DragFloat("Max Height", &new_terrain_max_height, 0.2f, 0.f, FLT_MAX);
                ImGui::DragFloat2("Extents", value_ptr(new_terrain_extents), 0.2, 0.f, FLT_MAX);
                ImGui::DragInt2("Resolution", value_ptr(new_terrain_resolution), 1.f, 1, 4096);
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("Camera")) {
                if (ImGui::IsItemClicked()) {
                    create_new_camera = true;
                }
                ImGui::DragFloat3("Position", value_ptr(new_camera_position), 0.2f, -FLT_MAX, FLT_MAX);
                ImGui::EndMenu();
            }

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

    if (const auto target_handle = entt::handle{ registry, signals.make_active.target }) {
        imgui::GenericMakeActive(target_handle);
    }

    if (const auto target_handle = entt::handle{ registry, signals.selection.target }) {

        if (signals.selection.toggle_mode) {
            switch_tag<Selected>(target_handle);
        } else {
            registry.clear<Selected>();
            set_tag<Selected>(target_handle);
        }
    }


    if (const auto target_handle = entt::handle{ registry, signals.detach_from_parent.target }) {
        if (has_parent(target_handle)) {
            detach_from_parent(target_handle);
        }
    }


    if (const auto target_handle = entt::handle{ registry, signals.attach_selected.target }) {

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


    if (const auto target_handle = entt::handle{ registry, signals.destroy.target }) {

        if (signals.destroy.with_descendants) {
            destroy_subtree(target_handle);
        } else {
            destroy_and_orphan_children(target_handle);
        }
    }


    if (create_new_node) {
        const entt::handle new_node{ registry, registry.create() };
        new_node.emplace<Transform>().translate(new_node_position);
    }

    if (create_new_plight) {
        const entt::handle new_plight{ registry, registry.create() };
        new_plight.emplace<PointLight>(new_plight_template);
        new_plight.emplace<Transform>().translate(new_plight_position);
        if (new_plight_cast_shadow) {
            set_tag<ShadowCasting>(new_plight);
        }
    }

    if (create_new_alight) {
        const entt::handle new_alight{ registry, registry.create() };
        new_alight.emplace<AmbientLight>(new_alight_template);
        if (!has_active<AmbientLight>(registry)) {
            make_active<AmbientLight>(new_alight);
        }
    }

    if (create_new_dlight) {
        const entt::handle new_dlight{ registry, registry.create() };
        new_dlight.emplace<DirectionalLight>(new_dlight_template);
        new_dlight.emplace<Transform>().orientation() =
            // TODO: Initial orientation?
            glm::quatLookAt(glm::vec3{ 0.f, -1.f, 0.f }, { 0.f, 0.f, -1.f });
        if (new_dlight_cast_shadow) {
            set_tag<ShadowCasting>(new_dlight);
        }
        if (!has_active<DirectionalLight>(registry)) {
            make_active<DirectionalLight>(new_dlight);
        }
    }

    if (create_new_terrain) {
        const entt::handle new_terrain{ registry, registry.create() };
        const Size2S   resolution{ size_t(new_terrain_resolution.x), size_t(new_terrain_resolution.y) };
        const Extent2F extents   { new_terrain_extents.x, new_terrain_extents.y };
        new_terrain.emplace<TerrainChunk>(create_terrain_chunk(new_terrain_max_height, extents, resolution));
        new_terrain.emplace<Transform>();
    }

    if (create_new_camera) {
        const entt::handle new_camera{ registry, registry.create() };
        new_camera.emplace<Camera>(Camera::Params{});
        new_camera.emplace<Transform>().translate(new_camera_position);
        if (!has_active<Camera>(registry)) {
            make_active<Camera>(new_camera);
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
