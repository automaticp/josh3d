#include "DefaultImporters.hpp"
#include "AssetImporter.hpp"


namespace josh {


void register_default_importers(AssetImporter& i) {
    i.register_importer<ImportSceneParams>  (&import_scene);
    i.register_importer<ImportTextureParams>(&import_texture);
}


} // namespace josh
