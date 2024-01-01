#include "ImGuiSelected.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "components/ChildMesh.hpp"
#include "components/Mesh.hpp"
#include "components/Name.hpp"
#include "components/Path.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/entity.hpp>
#include <imgui.h>


namespace josh {


void ImGuiSelected::display() {

    for (auto [e] : registry_.view<tags::Selected>().each()) {

        entt::handle handle{ registry_, e };

        // Deal with meshes.
        if (auto mesh = handle.try_get<components::Mesh>()) {
            imgui::MeshWidget(handle);
            if (auto as_child = handle.try_get<components::ChildMesh>()) {

                if (ImGui::TreeNode(void_id(e),
                    "Part of Model [%d]", entt::to_entity(as_child->parent)))
                {
                    imgui::ModelWidget({ registry_, as_child->parent });
                    ImGui::TreePop();
                }
            }
        }

        // TODO:
        // - Deal with lights;
        // - Deal with terrain chunks.

    }

}


} // namespace josh
