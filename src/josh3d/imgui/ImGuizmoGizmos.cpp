#include "ImGuizmoGizmos.hpp"
#include "ECSHelpers.hpp"
#include "components/Transform.hpp"
#include "tags/Selected.hpp"
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>


namespace josh {


void ImGuizmoGizmos::new_frame() {
    ImGuizmo::BeginFrame();
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    // ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
}


void ImGuizmoGizmos::display() {

    auto view = registry_.view<tags::Selected>();
    if (!view.empty()) {

        // TODO: Only handles ONE selected object.
        // We should also handle Mesh vs. Model selection.
        auto some_selected = entt::handle{ registry_, view.front() };

        if (auto transform = some_selected.try_get<components::Transform>()) {

            auto  world_transform = get_full_mesh_transform(some_selected, *transform);
            auto  transform_mat   = world_transform.mtransform().model();

            auto view_mat = cam_.view_mat();
            auto proj_mat = cam_.projection_mat();
            glm::mat4 delta_mat{};

            ImGuizmo::Manipulate(
                glm::value_ptr(view_mat),
                glm::value_ptr(proj_mat),
                ImGuizmo::TRANSLATE,
                ImGuizmo::WORLD,
                glm::value_ptr(transform_mat),
                glm::value_ptr(delta_mat)
            );

            glm::vec3 translation_delta;
            glm::vec3 rotation_delta;
            glm::vec3 scaling_delta;
            ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(delta_mat),
                glm::value_ptr(translation_delta),
                glm::value_ptr(rotation_delta),
                glm::value_ptr(scaling_delta)
            );

            // Should this be affecting child transform or the parent one?
            transform->translate(translation_delta);

        }
    }
}


} // namespace josh






