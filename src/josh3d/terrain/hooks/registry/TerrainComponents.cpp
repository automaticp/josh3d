#include "TerrainComponents.hpp"
#include "GLAPICommonTypes.hpp"
#include "GLObjectHelpers.hpp"
#include "GLTextures.hpp"
#include "Pixels.hpp"
#include "PixelData.hpp"
#include "AttributeTraits.hpp" // IWYU pragma: keep (traits)
#include "PixelPackTraits.hpp" // IWYU pragma: keep (traits)
#include "ImGuiHelpers.hpp"
#include "ImGuiComponentWidgets.hpp"
#include "NoiseGenerators.hpp"
#include "Size.hpp"
#include "TerrainGenerators.hpp"
#include "TerrainChunk.hpp"
#include "Transform.hpp"
#include <entt/entt.hpp>
#include <glbinding/gl/enum.h>
#include <imgui.h>


namespace josh::imguihooks::registry {


void TerrainComponents::operator()(entt::registry& registry) {

    if (ImGui::Button("Generate Chunk")) {

        Size2S size{ 256, 256 };
        PixelData<pixel::RedF> hdata{ size };
        static WhiteNoiseGenerator noise_generator;
        for (auto& px : hdata) {
            px.r = noise_generator();
        }

        auto mesh_data =
            generate_terrain_mesh(size, [&](const Index2S& idx) { return hdata.at(idx).r; });

        Mesh mesh{ mesh_data };

        auto sizei = Size2I{ size };
        UniqueTexture2D heightmap;
        heightmap->allocate_storage(sizei, InternalFormat::R32F, max_num_levels(sizei));
        heightmap->upload_image_region({ {}, sizei }, hdata.data());
        heightmap->generate_mipmaps();

        heightmap->set_sampler_min_mag_filters(MinFilter::NearestMipmapLinear, MagFilter::Nearest);

        entt::handle handle{ registry, registry.create() };
        handle.emplace<Transform>();
        handle.emplace<TerrainChunk>(
            std::move(mesh), std::move(hdata), std::move(heightmap)
        );

    }

    auto to_remove = on_value_change_from<entt::entity>(
        entt::null,
        [&](entt::entity e) { registry.destroy(e); }
    );

    for (auto [e, transform, chunk]
        : registry.view<Transform, TerrainChunk>().each())
    {
        ImGui::PushID(void_id(e));

        const bool display_node =
            ImGui::TreeNode(void_id(e), "Terrain Chunk %d", entt::to_entity(e));

        ImGui::SameLine();
        if (ImGui::SmallButton("Remove")) {
            to_remove.set(e);
        }

        if (display_node) {
            imgui::TransformWidget(&transform);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

}


} // namespace josh::imguihooks::registry
