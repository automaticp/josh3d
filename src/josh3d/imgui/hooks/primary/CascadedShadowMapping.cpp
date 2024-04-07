#include "detail/SimpleStageHookMacro.hpp"
#include "CascadedShadowMapping.hpp"
#include <imgui.h>


JOSH3D_SIMPLE_STAGE_HOOK_BODY(primary, CascadedShadowMapping) {

    ImGui::Checkbox("Face Culling", &stage_.enable_face_culling);


    ImGui::BeginDisabled(!stage_.enable_face_culling);

    const char* face_names[] = {
        "Back",
        "Front",
    };

    int face = stage_.faces_to_cull == Face::Back ? 0 : 1;
    if (ImGui::ListBox("Faces to Cull", &face, face_names, 2, 2)) {
        stage_.faces_to_cull = face == 0 ? Face::Back : Face::Front;
    }

    ImGui::EndDisabled();
}
