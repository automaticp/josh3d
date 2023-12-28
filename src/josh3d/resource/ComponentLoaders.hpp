#pragma once
#include "Filesystem.hpp"
#include "Pixels.hpp"
#include "TextureHelpers.hpp"
#include "components/Skybox.hpp"
#include <entt/entt.hpp>


namespace josh {


inline components::Skybox& load_skybox_into(
    entt::handle skybox_handle, const File& skybox_json)
{
        auto data = load_cubemap_from_json<pixel::RGBA>(skybox_json);
        auto& skybox =
            skybox_handle.emplace_or_replace<components::Skybox>(std::make_shared<UniqueCubemap>());

        using enum GLenum;
        skybox.cubemap->bind()
            .and_then([&](BoundCubemap<GLMutable>& cubemap) {
                attach_data_to_cubemap_as_skybox(cubemap, data, GL_SRGB_ALPHA);
            })
            .unbind();

        return skybox;
}


} // namespace josh
