#include "ImGuiAssetBrowser.hpp"
#include "AssetLoader.hpp"
#include <imgui.h>


namespace josh {


void ImGuiAssetBrowser::display() {

    if (ImGui::CollapsingHeader("Models")) {

        asset_loader_.peek_model_cache([](const AssetLoader::ModelCache& cache) {
            for (const auto& [path, asset] : cache) {
                ImGui::TextUnformatted(path.entry().c_str());
                ImGui::SameLine();
                ImGui::TextUnformatted(path.subpath().begin(), path.subpath().end());
            }
        });
    }

    if (ImGui::CollapsingHeader("Meshes")) {
        asset_loader_.peek_mesh_cache([](const AssetLoader::MeshCache& cache) {
            for (const auto& [path, asset] : cache) {
                ImGui::TextUnformatted(path.entry().c_str());
                ImGui::SameLine();
                ImGui::TextUnformatted(path.subpath().begin(), path.subpath().end());
            }
        });
    }

    if (ImGui::CollapsingHeader("Textures")) {
        asset_loader_.peek_texture_cache([](const AssetLoader::TextureCache& cache) {
            for (const auto& [path, asset] : cache) {
                ImGui::TextUnformatted(path.entry().c_str());
                ImGui::SameLine();
                ImGui::TextUnformatted(path.subpath().begin(), path.subpath().end());
            }
        });
    }

}



} // namespace josh
