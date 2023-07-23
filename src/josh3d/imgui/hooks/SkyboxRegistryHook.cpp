#include "SkyboxRegistryHook.hpp"
#include "CubemapData.hpp"
#include "GLTextures.hpp"
#include "RenderComponents.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <memory>
#include <stdexcept>



namespace josh::imguihooks {


void SkyboxRegistryHook::operator()(entt::registry& registry) {

    if (ImGui::TreeNode("Load Skybox")) {
        ImGui::InputText("Directory", &load_path_);

        ImGui::TextUnformatted("Filenames:");

        constexpr const char* side_names[]{ "+X", "-X", "+Y", "-Y", "+Z", "-Z" };

        for (size_t i{ 0 }; i < 6; ++i) {
            ImGui::PushID(int(i));
            ImGui::InputText(side_names[i], &filenames_[i]);
            ImGui::PopID();
        }

        if (ImGui::Button("Load")) {
            try {
                File files[6]{
                    File(Path(load_path_) / filenames_[0]),
                    File(Path(load_path_) / filenames_[1]),
                    File(Path(load_path_) / filenames_[2]),
                    File(Path(load_path_) / filenames_[3]),
                    File(Path(load_path_) / filenames_[4]),
                    File(Path(load_path_) / filenames_[5]),
                };
                auto data = CubemapData::from_files(files);
                auto skybox_e = registry.view<components::Skybox>().back();
                if (skybox_e == entt::null) {
                    skybox_e = registry.create();
                }
                auto& skybox =
                    registry.emplace_or_replace<components::Skybox>(skybox_e, std::make_shared<Cubemap>());

                using enum GLenum;
                skybox.cubemap->bind()
                    .attach_data(data, GL_SRGB_ALPHA)
                    .unbind();

                error_text_ = "";
            } catch (const std::runtime_error& e) {
                error_text_ = e.what();
            }
        }

        ImGui::TextUnformatted(error_text_.c_str());

        ImGui::TreePop();
    }

}



} // namespace josh::imguihooks
