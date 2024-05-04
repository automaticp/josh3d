#include "ImGuiSelected.hpp"
#include "ECSHelpers.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "LightCasters.hpp"
#include "components/ChildMesh.hpp"
#include "components/Mesh.hpp"
#include "components/Model.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/entity.hpp>
#include <imgui.h>


namespace josh {


void ImGuiSelected::display() {

    auto destroy_other = [](entt::handle handle) { handle.destroy(); };

    auto to_remove_model = on_value_change_from<entt::handle>({}, &destroy_model);
    auto to_remove_other = on_value_change_from<entt::handle>({}, +destroy_other);

    for (auto [e] : registry_.view<tags::Selected>().each()) {

        entt::handle handle{ registry_, e };

        if (auto mesh = handle.try_get<components::Mesh>()) {
            // Meshes.
            imgui::MeshWidget(handle);
            if (auto as_child = handle.try_get<components::ChildMesh>()) {

                entt::handle parent_handle{ registry_, as_child->parent };
                if (ImGui::TreeNode(void_id(e),
                    "Part of Model [%d]", entt::to_entity(parent_handle.entity())))
                {
                    if (imgui::ModelWidget(parent_handle) == imgui::Feedback::Remove) {
                        to_remove_model.set(parent_handle);
                    }
                    ImGui::TreePop();
                }
            }
        } else if (auto model = handle.try_get<components::Model>()) {
            // Models.
            if (imgui::ModelWidget(handle) == imgui::Feedback::Remove) {
                to_remove_model.set(handle);
            }
        } else if (auto plight = handle.try_get<light::Point>()) {
            // Point Lights.
            if (imgui::PointLightWidget(handle) == imgui::Feedback::Remove) {
                to_remove_other.set(handle);
            }
        } else {
            ImGui::Text("Unknown Entity [%d]", entt::to_entity(handle.entity()));
        }

        // TODO:
        // - Deal with lights;
        // - Deal with terrain chunks.

    }

}


} // namespace josh
