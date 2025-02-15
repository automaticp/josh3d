#include "ImGuiSelected.hpp"
#include "Camera.hpp"
#include "Components.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "LightCasters.hpp"
#include "Mesh.hpp"
#include "SkinnedMesh.hpp"
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
            imgui::TransformWidget(*transform);
        }

        // Mostly for debugging.
        if (display_model_matrix) {
            if (auto mtf = handle.try_get<MTransform>()) {
                imgui::Matrix4x4DisplayWidget(mtf->model());
            }
        }

        if (has_component<Mesh>(handle)) {
            imgui::MaterialsWidget(handle);
        }

        if (has_component<SkinnedMesh>(handle)) {
            imgui::MaterialsWidget(handle);
            imgui::AnimationsWidget(handle);
        }

        if (has_component<PointLight>(handle)) {
            imgui::PointLightHandleWidget(handle);
        }

        if (has_component<DirectionalLight>(handle)) {
            imgui::DirectionalLightHandleWidget(handle);
        }

        if (has_component<AmbientLight>(handle)) {
            imgui::AmbientLightHandleWidget(handle);
        }

        if (has_component<Camera>(handle)) {
            imgui::CameraHandleWidget(handle);
        }


        ImGui::Separator();

        ImGui::PopID();
    }

}


} // namespace josh
