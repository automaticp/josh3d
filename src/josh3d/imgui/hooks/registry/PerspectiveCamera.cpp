#include "PerspectiveCamera.hpp"
#include <entt/entity/fwd.hpp>
#include <imgui.h>



namespace josh::imguihooks::registry {


void PerspectiveCamera::operator()(entt::registry&) {

    const auto& params = cam_.get_params();
    float z_near_far[2]{ params.z_near, params.z_far };
    if (ImGui::SliderFloat2("Z Near/Far", z_near_far,
        0.01f, 10000.f, "%.2f", ImGuiSliderFlags_Logarithmic))
    {
        auto params_copy = params;
        params_copy.z_near = z_near_far[0];
        params_copy.z_far  = z_near_far[1];
        cam_.update_params(params_copy);
    }


    ImGui::DragFloat3(
        "World Pos.", glm::value_ptr(cam_.transform.position()),
        1.f, -FLT_MAX, FLT_MAX, "%.1f"
    );

}


} // namespace josh::imguihooks::registry
