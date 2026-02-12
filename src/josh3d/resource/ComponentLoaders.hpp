#pragma once
#include "Asset.hpp"
#include "ECS.hpp"
#include "Filesystem.hpp"
#include "components/Skybox.hpp"


/*
TODO: Deprecate.
*/
namespace josh {


auto load_skybox_into(
    Handle      handle,
    const File& skybox_json)
        -> Skybox&;


void emplace_model_asset_into(
    Handle           model_handle,
    SharedModelAsset model_asset);


} // namespace josh
