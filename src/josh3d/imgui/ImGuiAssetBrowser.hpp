#pragma once
#include "AssetLoader.hpp"


namespace josh {


class ImGuiAssetBrowser {
public:
    ImGuiAssetBrowser(AssetLoader& asset_loader) : asset_loader_{ asset_loader } {}

    void display();

private:
    AssetLoader& asset_loader_;
};


} // namespace josh
