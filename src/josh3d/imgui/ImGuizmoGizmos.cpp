#include "ImGuizmoGizmos.hpp"
#include "Transform.hpp"
#include "components/ChildMesh.hpp"
#include "components/Transform.hpp"
#include "tags/Selected.hpp"
#include <ImGuizmo.h>
#include <entt/entity/fwd.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <cassert>


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
        auto some_selected = entt::handle{ registry_, view.front() };

        if (auto transform = some_selected.try_get<components::Transform>()) {

            auto mtransform = transform->mtransform();

            auto view_mat = cam_.view_mat();
            auto proj_mat = cam_.projection_mat();

            glm::mat4 delta_mat{};

            ImGuizmo::OPERATION op = [&] {
                switch (active_operation) {
                    using enum GizmoOperation;
                    case translation: return ImGuizmo::TRANSLATE;
                    case rotation:    return ImGuizmo::ROTATE;
                    case scaling:     return ImGuizmo::SCALE;
                    default:          assert(false);
                }
            }();

            if (!some_selected.all_of<components::ChildMesh>()) {
                // FIXME: This branch is redundant and should be rewritten
                // to handle models, orphaned meshes and child meshes in one go.

                auto transform_mat = mtransform.model();

                ImGuizmo::Manipulate(
                    glm::value_ptr(view_mat),
                    glm::value_ptr(proj_mat),
                    op,
                    ImGuizmo::WORLD,
                    glm::value_ptr(transform_mat),
                    glm::value_ptr(delta_mat)
                );

                glm::vec3 new_translation;
                glm::vec3 new_rotation;
                glm::vec3 new_scaling;
                ImGuizmo::DecomposeMatrixToComponents(
                    glm::value_ptr(transform_mat),
                    glm::value_ptr(new_translation),
                    glm::value_ptr(new_rotation),
                    glm::value_ptr(new_scaling)
                );

                switch (active_operation) {
                    case GizmoOperation::translation:
                        transform->position() = new_translation;
                        break;
                    case GizmoOperation::rotation:
                        transform->rotation() = glm::quat(glm::radians(new_rotation));
                        break;
                    case GizmoOperation::scaling:
                        // FIXME: Scaling does not seem to work correctly, investigate later.
                        transform->scaling()  = new_scaling;
                        break;
                }

            } else /* is a ChildMesh */ {
                entt::entity parent_ent    = some_selected.get<components::ChildMesh>().parent;
                const Transform& parent_tf = registry_.get<components::Transform>(parent_ent);
                glm::mat4 parent_mat       = parent_tf.mtransform().model();
                glm::mat4 child_mat        = mtransform.model();

                glm::mat4 world_mat        = parent_mat * child_mat;

                ImGuizmo::Manipulate(
                    glm::value_ptr(view_mat),
                    glm::value_ptr(proj_mat),
                    op,
                    ImGuizmo::WORLD,
                    glm::value_ptr(world_mat),
                    glm::value_ptr(delta_mat) // Use it when transforming multiple objects at once
                );


                // Local -> World -> Apply Delta -> Local -> Set Position, Orientation, Scale

                glm::mat4 new_local_mat = glm::inverse(parent_mat) * world_mat;
                glm::vec3 new_local_translation;
                glm::vec3 new_local_rotation;
                glm::vec3 new_local_scaling;
                ImGuizmo::DecomposeMatrixToComponents(
                    glm::value_ptr(new_local_mat),
                    glm::value_ptr(new_local_translation),
                    glm::value_ptr(new_local_rotation),
                    glm::value_ptr(new_local_scaling)
                );

                switch (active_operation) {
                    case GizmoOperation::translation:
                        transform->position() = new_local_translation;
                        break;
                    case GizmoOperation::rotation:
                        transform->rotation() = glm::quat(glm::radians(new_local_rotation));
                        break;
                    case GizmoOperation::scaling:
                        transform->scaling()  = new_local_scaling;
                        break;
                }


            }

        }
    }
}


} // namespace josh






