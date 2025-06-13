#include "ImGuiSelected.hpp"
#include "Camera.hpp"
#include "Components.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "LightCasters.hpp"
#include "Materials.hpp"
#include "Mesh.hpp"
#include "SkinnedMesh.hpp"
#include "Transform.hpp"
#include "tags/Selected.hpp"
#include <entt/entity/entity.hpp>
#include <imgui.h>


namespace josh {


void ImGuiSelected::display()
{
    for (const Entity entity : registry.view<Selected>())
    {
        ImGui::PushID(void_id(entity));
        const Handle handle = { registry, entity };

        imgui::GenericHeaderText(handle);

        // Display Transform independent of other components.
        if (auto* transform = handle.try_get<Transform>())
            imgui::TransformWidget(*transform);

        // Mostly for debugging.
        if (display_model_matrix)
            if (auto* mtf = handle.try_get<MTransform>())
                imgui::Matrix4x4DisplayWidget(mtf->model());

        if (has_component<Mesh>(handle) or
            has_component<SkinnedMesh>(handle) or
            handle.any_of<MaterialDiffuse, MaterialNormal, MaterialSpecular>())
        {
            imgui::MaterialsWidget(handle);
        }

        if (has_component<SkinnedMesh>(handle))
            imgui::AnimationsWidget(handle);

        if (has_component<PointLight>(handle))
            imgui::PointLightHandleWidget(handle);

        if (has_component<DirectionalLight>(handle))
            imgui::DirectionalLightHandleWidget(handle);

        if (has_component<AmbientLight>(handle))
            imgui::AmbientLightHandleWidget(handle);

        if (has_component<Camera>(handle))
            imgui::CameraHandleWidget(handle);

        ImGui::Separator();

        if (display_all_components)
        {
            for (auto&& [index, storage] : handle.storage())
            {
                const auto name = storage.type().name();
                ImGui::Text("%.*s", int(name.size()), name.data());
            }
            ImGui::Separator();
        }

        ImGui::PopID();
    }
}


} // namespace josh
