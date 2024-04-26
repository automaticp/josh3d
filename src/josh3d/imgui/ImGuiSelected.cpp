#include "ImGuiSelected.hpp"
#include "ECSHelpers.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "components/ChildMesh.hpp"
#include "components/Mesh.hpp"
#include "components/Model.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/entity.hpp>
#include <imgui.h>


namespace josh {


void ImGuiSelected::display() {

    auto to_remove = on_value_change_from<entt::handle>({}, &destroy_model);

    for (auto [e] : registry_.view<tags::Selected>().each()) {

        entt::handle handle{ registry_, e };

        // Deal with meshes.
        if (auto mesh = handle.try_get<components::Mesh>()) {
            imgui::MeshWidget(handle);
            if (auto as_child = handle.try_get<components::ChildMesh>()) {

                entt::handle parent_handle{ registry_, as_child->parent };
                if (ImGui::TreeNode(void_id(e),
                    "Part of Model [%d]", entt::to_entity(parent_handle.entity())))
                {
                    if (imgui::ModelWidget(parent_handle) == imgui::Feedback::Remove) {
                        to_remove.set(parent_handle);
                    }
                    ImGui::TreePop();
                }
            }
        } else if (auto model = handle.try_get<components::Model>()) {
            if (imgui::ModelWidget(handle) == imgui::Feedback::Remove) {
                to_remove.set(handle);
            }
        }

        // TODO:
        // - Deal with lights;
        // - Deal with terrain chunks.

    }

}


} // namespace josh
