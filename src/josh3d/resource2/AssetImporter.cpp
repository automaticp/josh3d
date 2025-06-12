#include "AssetImporter.hpp"
#include "RuntimeError.hpp"


namespace josh {


auto AssetImporter::_import_asset(const key_type& key, Path path, AnyRef params)
    -> Job<UUID>
{
    if (auto* kv = try_find(dispatch_table_, key))
    {
        auto& importer = kv->second;
        return importer(AssetImporterContext(*this), MOVE(path), params);
    }
    throw_fmt("No importer found for ParamType {}", key.pretty_name());
}


} // namespace josh
