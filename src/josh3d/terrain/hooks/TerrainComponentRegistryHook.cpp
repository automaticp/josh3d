#include "TerrainComponentRegistryHook.hpp"
#include "HeightmapData.hpp"
#include "ImGuiHelpers.hpp"
#include "NoiseGenerators.hpp"
#include "Size.hpp"
#include "components/TerrainChunk.hpp"
#include "Transform.hpp"
#include <algorithm>
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <imgui.h>


namespace josh::imguihooks {


void TerrainComponentRegistryHook::operator()(entt::registry& registry) {

    if (ImGui::Button("Generate Chunk")) {

        Size2S size{ 256, 256 };
        HeightmapData hdata{ size };
        static WhiteNoiseGenerator noise_generator;
        for (float& pix : hdata) { pix = noise_generator(); }


        auto mesh_data =
            generate_terrain_mesh(size, [&](size_t x, size_t y) { return hdata.at(x, y); });

        Mesh mesh{ mesh_data };


        using enum GLenum;
        UniqueTexture2D heightmap;
        heightmap.bind()
            .specify_image(
                Size2I{ hdata.image_size() },
                TexSpec{ GL_R32F },
                TexPackSpec{ GL_RED, GL_FLOAT },
                hdata.data()
            )
            .set_min_mag_filters(GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST)
            .generate_mipmaps()
            .unbind();


        entt::handle handle{ registry, registry.create() };
        handle.emplace<Transform>();
        handle.emplace<components::TerrainChunk>(
            std::move(hdata), std::move(mesh), std::move(heightmap)
        );

    }


    for (auto [e, transform, chunk]
        : registry.view<Transform, components::TerrainChunk>().each())
    {
        if (ImGui::TreeNode(void_id(e), "Terrain Chunk %d", entt::to_entity(e))) {

            ImGui::TransformWidget(&transform);

            ImGui::TreePop();
        }
    }

}


} // namespace josh::imguihooks
