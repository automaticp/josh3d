#include "ImGuiSelected.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "components/Name.hpp"
#include "tags/Selected.hpp"
#include <imgui.h>


namespace josh {


void ImGuiSelected::display() {

    for (auto [e] : registry_.view<tags::Selected>().each()) {
        ImGui::Text("Selected object [%d]", entt::to_entity(e));

        entt::handle handle{ registry_, e };

        if (auto name = handle.try_get<components::Name>()) {
            ImGui::NameWidget(name);
        }

        if (auto transform = handle.try_get<components::Transform>()) {
            ImGui::TransformWidget(transform);
        }
    }

}


} // namespace josh
