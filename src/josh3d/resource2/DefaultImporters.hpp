#pragma once
#include "AssetImporter.hpp"
#include "DefaultResourceFiles.hpp"
#include "Coroutines.hpp"
#include "Filesystem.hpp"
#include "UUID.hpp"


/*
NOTE: Most of the implementation has been moved into detail/ and split
into multiple files to preserve compile times and my sanity. Poor clangd
would take 3s to respond otherwise. I blame ranges.
*/
namespace josh {


class AssetImporterContext;


struct ImportTextureParams {
    // TODO: Only RAW and PNG is supported for now.
    // This should also specify Mixed mode at least.
    // So it probably needs a new enum.
    TextureFile::Encoding   encoding;
    TextureFile::Colorspace colorspace;
    bool                    generate_mips = true;
};


auto import_texture(
    AssetImporterContext context,
    Path                 path,
    ImportTextureParams  params)
        -> Job<UUID>;


struct ImportSceneParams {
    TextureFile::Encoding texture_encoding = TextureFile::Encoding::PNG;
    bool generate_mips    = true;
    // bool skip_meshes     = false;
    // bool skip_textures   = false;
    // bool skip_skeletons  = false;
    // bool skip_animations = false;
    bool collapse_graph   = false; // Equivalent to aiProcess_OptimizeGraph
    bool merge_meshes     = false; // Equivalent to aiProcess_OptimizeMeshes
};


auto import_scene(
    AssetImporterContext context,
    Path                 path,
    ImportSceneParams    params)
        -> Job<UUID>;


inline void register_default_importers(
    AssetImporter& i)
{
    i.register_importer<ImportSceneParams>  (&import_scene);
    i.register_importer<ImportTextureParams>(&import_texture);
}


} // namespace josh
