#include "SkyboxRegistryHook.hpp"
#include "CubemapData.hpp"
#include "Filesystem.hpp"
#include "GLTextures.hpp"
#include "Pixels.hpp"
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
        ImGui::InputText("Cubemap JSON", &load_path_);

        if (ImGui::Button("Load")) {
            try {
                Path path{ load_path_ };
                File skybox_json = std::invoke([&]() -> File {
                    if (path.is_relative()) {
                        return VPath(path);
                    } else {
                        return File(path);
                    }
                });
                auto data = load_cubemap_from_json<pixel::RGBA>(skybox_json);

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
