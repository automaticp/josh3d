#include "SkyboxRegistryHook.hpp"
#include "CubemapData.hpp"
#include "Filesystem.hpp"
#include "GLTextures.hpp"
#include "TextureHelpers.hpp"
#include "components/Skybox.hpp"
#include "VPath.hpp"
#include <entt/entity/entity.hpp>
#include <entt/entt.hpp>
#include <functional>
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
                Path path{ load_path_ };
                Directory skybox_dir = std::invoke([&]() -> Directory {
                    if (path.is_relative()) {
                        return VPath(path);
                    } else {
                        return Directory(path);
                    }
                });
                File files[6]{
                    File(skybox_dir.path() / filenames_[0]),
                    File(skybox_dir.path() / filenames_[1]),
                    File(skybox_dir.path() / filenames_[2]),
                    File(skybox_dir.path() / filenames_[3]),
                    File(skybox_dir.path() / filenames_[4]),
                    File(skybox_dir.path() / filenames_[5]),
                };
                auto data = CubemapData::from_files(files);
                auto skybox_e = registry.view<components::Skybox>().back();
                if (skybox_e == entt::null) {
                    skybox_e = registry.create();
                }
                auto& skybox =
                    registry.emplace_or_replace<components::Skybox>(skybox_e, std::make_shared<UniqueCubemap>());

                using enum GLenum;
                skybox.cubemap->bind()
                    .and_then([&](BoundCubemap<GLMutable>& cubemap) {
                        attach_data_to_cubemap(cubemap, data, GL_SRGB_ALPHA);
                    })
                    // FIXME: Find a better place for this.
                    .set_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
                    .set_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
                    .set_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
                    .set_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
                    .set_parameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE)
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
