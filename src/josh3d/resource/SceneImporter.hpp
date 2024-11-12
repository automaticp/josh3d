#pragma once
#include "AssetImporter.hpp"
#include "UniqueFunction.hpp"
#include <entt/entity/fwd.hpp>
#include <nlohmann/json_fwd.hpp>


namespace josh {


/*
Deserializes JSON into the scene registry.
*/
class SceneImporter {
public:
    using TypeImporter = UniqueFunction<void(const nlohmann::json&, entt::handle)>;

    SceneImporter(AssetImporter& asset_importer, entt::registry& registry);

    void import_from_json_file(const Path& json_file);
    void import_from_json(const nlohmann::json& json);
    void register_importer(std::string_view type, TypeImporter importer);

private:
    entt::registry&                               registry_;
    std::unordered_map<std::string, TypeImporter> type_importers_;
};


} // namespace josh
