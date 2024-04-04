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

        UniqueCubemap cubemap =
            create_skybox_from_cubemap_data(data, InternalFormat::SRGBA8);

        auto& skybox =
            skybox_handle.emplace_or_replace<components::Skybox>(std::move(cubemap));

        return skybox;
}


} // namespace josh
