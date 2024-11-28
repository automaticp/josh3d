#pragma once
#include "AssetManager.hpp"


namespace josh {


class ImGuiAssetBrowser {
public:
    ImGuiAssetBrowser(AssetManager& asset_manager) : asset_manager_{ asset_manager } {}

    // TODO: Does nothing for now. The AssetManager does not expose anything for browsing yet.
    void display();

private:
    AssetManager& asset_manager_;
};


} // namespace josh
