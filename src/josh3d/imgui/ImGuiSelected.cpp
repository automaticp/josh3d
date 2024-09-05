#include "ImGuiSelected.hpp"
#include "Active.hpp"
#include "Components.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "LightCasters.hpp"
#include "Mesh.hpp"
#include "Transform.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/entity.hpp>
#include <imgui.h>


namespace josh {


void ImGuiSelected::display() {

    for (auto entity : registry_.view<Selected>()) {
        ImGui::PushID(void_id(entity));
        const entt::handle handle{ registry_, entity };


        imgui::GenericHeaderText(handle);

        // Display Transform independent of other components.
        if (auto transform = handle.try_get<Transform>()) {
            imgui::TransformWidget(transform);
        }

        // Mostly for debugging.
        if (display_model_matrix) {
            if (auto mtf = handle.try_get<MTransform>()) {
                imgui::Matrix4x4DisplayWidget(mtf->model());
            }
        }

        if (auto mesh = handle.try_get<Mesh>()) {
            imgui::MaterialsWidget(handle);
        }

        if (auto plight = handle.try_get<PointLight>()) {
            imgui::PointLightWidgetBody(handle);
        }

        if (auto dlight = handle.try_get<DirectionalLight>()) {
            imgui::DirectionalLightWidget(handle);
        }

        if (auto alight = handle.try_get<AmbientLight>()) {
            imgui::AmbientLightWidget(alight);
        }

        ImGui::Separator();


        ImGui::PopID();
    }

}


} // namespace josh
