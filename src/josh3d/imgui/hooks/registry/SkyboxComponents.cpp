#include "SkyboxComponents.hpp"
#include "ComponentLoaders.hpp"
#include "Filesystem.hpp"
#include "ImGuiHelpers.hpp"
#include "components/Skybox.hpp"
#include "VPath.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entt.hpp>
#include <functional>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <stdexcept>


namespace josh::imguihooks::registry {


void SkyboxComponents::operator()(entt::registry& registry) {

    auto try_load_skybox = [&] {
        try {
            error_text_ = "";

            Path path{ load_path_ };
            // TODO: emplace Path and VPath (optionally) like for models.
            File skybox_json = std::invoke([&]() -> File {
                if (path.is_relative()) {
                    return VPath(path);
                } else {
                    return File(path);
                }
            });

            auto skybox_e = registry.view<components::Skybox>().back();
            if (skybox_e == entt::null) {
                skybox_e = registry.create();
            }

            load_skybox_into({ registry, skybox_e }, skybox_json);

        } catch (const std::runtime_error& e) {
            error_text_ = e.what();
        }
    };

    auto load_skybox_signal = on_value_change_from(false, try_load_skybox);

    if (ImGui::InputText("##Path or VPath", &load_path_,
        ImGuiInputTextFlags_EnterReturnsTrue))
    {
        load_skybox_signal.set(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load from JSON")) {
        load_skybox_signal.set(true);
    }

    ImGui::TextColored({ 1.0f, 0.5f, 0.5f, 1.0f }, "%s", error_text_.c_str());

}


} // namespace josh::imguihooks::registry
