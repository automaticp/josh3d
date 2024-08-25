#include "ImGuizmoGizmos.hpp"
#include "Transform.hpp"
#include "ChildMesh.hpp"
#include "Transform.hpp"
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
}


void ImGuizmoGizmos::display() {

    auto view = registry_.view<Selected>();
    if (!view.empty()) {

        // TODO: Only handles ONE selected object.
        auto some_selected = entt::handle{ registry_, view.front() };

        if (auto transform = some_selected.try_get<Transform>()) {

            const glm::mat4 view_mat = cam_.view_mat();
            const glm::mat4 proj_mat = cam_.projection_mat();

            ImGuizmo::OPERATION op = [&] {
                switch (active_operation) {
                    using enum GizmoOperation;
                    case translation: return ImGuizmo::TRANSLATE;
                    case rotation:    return ImGuizmo::ROTATE;
                    case scaling:     return ImGuizmo::SCALE;
                    default:          assert(false);
                }
            }();

            ImGuizmo::MODE mode = [&] {
                switch (active_space) {
                    case GizmoSpace::world:
                        if (active_operation != GizmoOperation::scaling) {
                            return ImGuizmo::WORLD;
                        } else /* op is scaling */ {
                            // Only local scaling makes sense, since non-local scaling
                            // (in a rotated basis) is not representable in Transform.
                            //
                            // Blender does some funky stuff to support "world-space scaling"
                            // but it's not what you'd expect (i.e. not skew) and is not intuitive either way.
                            return ImGuizmo::LOCAL;
                        }
                    case GizmoSpace::local:
                        return ImGuizmo::LOCAL;
                    default:
                        assert(false);
                }
            }();


            /*
                [As Root]:
                    root_mat
                        -> Manipulate(root_mat)
                            -> Assign New TRS = Decompose(root_mat)

                [As Child]:
                    child_mat
                        -> world_mat = parent_mat * child_mat
                            -> Manipulate(world_mat)
                                -> new_child_mat = parent_mat^-1 * world_mat
                                    -> Assign New TRS = Decompose(new_child_mat)
            */


            if (!some_selected.all_of<ChildMesh>()) /* is a root or orphaned object */ {

                // TODO: This is kinda redundant with the other branch repeating
                // most of the code here, but whatever for now.

                glm::mat4 root_mat = transform->mtransform().model();

                ImGuizmo::Manipulate(
                    glm::value_ptr(view_mat),
                    glm::value_ptr(proj_mat),
                    op,
                    mode,
                    glm::value_ptr(root_mat)
                );

                glm::vec3 new_translation;
                glm::vec3 new_rotation;
                glm::vec3 new_scaling;
                ImGuizmo::DecomposeMatrixToComponents(
                    glm::value_ptr(root_mat),
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
                        transform->scaling()  = new_scaling;
                        break;
                }

            } else /* is a child Mesh */ {

                const entt::entity parent_ent = some_selected.get<ChildMesh>().parent;
                const glm::mat4 parent_mat    = registry_.get<Transform>(parent_ent).mtransform().model();
                const glm::mat4 child_mat     = transform->mtransform().model();

                glm::mat4 world_mat = parent_mat * child_mat;

                ImGuizmo::Manipulate(
                    glm::value_ptr(view_mat),
                    glm::value_ptr(proj_mat),
                    op,
                    mode,
                    glm::value_ptr(world_mat)
                );

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






