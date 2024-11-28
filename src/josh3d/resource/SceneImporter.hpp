#pragma once
#include "UniqueFunction.hpp"
#include "AssetManager.hpp"
#include "AssetUnpacker.hpp"
#include "ECS.hpp"
#include <nlohmann/json_fwd.hpp>


namespace josh {


/*
Deserializes JSON into the scene registry.
*/
class SceneImporter {
public:
    using TypeImporter = UniqueFunction<void(const nlohmann::json&, Handle)>;

    SceneImporter(
        AssetManager&  asset_manager,
        AssetUnpacker& asset_unpacker,
        Registry&      registry);

    void import_from_json_file(const Path& json_file);
    void import_from_json(const nlohmann::json& json);
    void register_importer(std::string_view type, TypeImporter importer);

private:
    Registry&                                     registry_;
    std::unordered_map<std::string, TypeImporter> type_importers_;
};


} // namespace josh
