#include "TerrainComponents.hpp"
#include "ImageData.hpp"
#include "Pixels.hpp"
#include "ImGuiHelpers.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "NoiseGenerators.hpp"
#include "Size.hpp"
#include "TerrainGenerators.hpp"
#include "components/TerrainChunk.hpp"
#include "Transform.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <imgui.h>


namespace josh::imguihooks::registry {


void TerrainComponents::operator()(entt::registry& registry) {

    if (ImGui::Button("Generate Chunk")) {

        Size2S size{ 256, 256 };
        ImageData<pixel::REDF> hdata{ size };
        static WhiteNoiseGenerator noise_generator;
        for (auto& px : hdata) { px.r = noise_generator(); }


        auto mesh_data =
            generate_terrain_mesh(size, [&](const Index2S& idx) { return hdata.at(idx).r; });

        Mesh mesh{ mesh_data };


        using enum GLenum;
        UniqueTexture2D heightmap;
        heightmap.bind()
            .specify_image(
                Size2I{ hdata.size() },
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

            imgui::TransformWidget(&transform);

            ImGui::TreePop();
        }
    }

}


} // namespace josh::imguihooks::registry
