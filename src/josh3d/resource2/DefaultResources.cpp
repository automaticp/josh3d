#include "DefaultResources.hpp"
#include "AssetImporter.hpp"
#include "ResourceInfo.hpp"
#include "ResourceLoader.hpp"
#include "ResourceRegistry.hpp"
#include "ResourceUnpacker.hpp"


namespace josh {


void register_default_resource_info(ResourceInfo& m)
{
    m.register_resource_type<RT::Scene>();
    m.register_resource_type<RT::Material>();
    m.register_resource_type<RT::MeshDesc>();
    m.register_resource_type<RT::StaticMesh>();
    m.register_resource_type<RT::SkinnedMesh>();
    m.register_resource_type<RT::Texture>();
    m.register_resource_type<RT::Skeleton>();
    m.register_resource_type<RT::Animation>();
}

void register_default_resource_storage(ResourceRegistry& r)
{
    r.initialize_storage_for<RT::Scene>();
    r.initialize_storage_for<RT::MeshDesc>();
    r.initialize_storage_for<RT::Material>();
    r.initialize_storage_for<RT::StaticMesh>();
    r.initialize_storage_for<RT::SkinnedMesh>();
    r.initialize_storage_for<RT::Texture>();
    r.initialize_storage_for<RT::Skeleton>();
    r.initialize_storage_for<RT::Animation>();
}

void register_default_loaders(ResourceLoader& l)
{
    l.register_loader<RT::Scene>      (&load_scene);
    l.register_loader<RT::MeshDesc>   (&load_mdesc);
    l.register_loader<RT::Material>   (&load_material);
    l.register_loader<RT::StaticMesh> (&load_static_mesh);
    l.register_loader<RT::SkinnedMesh>(&load_skinned_mesh);
    l.register_loader<RT::Texture>    (&load_texture);
    l.register_loader<RT::Skeleton>   (&load_skeleton);
    l.register_loader<RT::Animation>  (&load_animation);
}

void register_default_unpackers(ResourceUnpacker& u)
{
    u.register_unpacker<RT::Scene,       Handle>(&unpack_scene);
    u.register_unpacker<RT::MeshDesc,    Handle>(&unpack_mdesc);
    u.register_unpacker<RT::Material,    Handle>(&unpack_material);
    u.register_unpacker<RT::StaticMesh,  Handle>(&unpack_static_mesh);
    u.register_unpacker<RT::SkinnedMesh, Handle>(&unpack_skinned_mesh);
}

void register_default_importers(AssetImporter& i)
{
    i.register_importer<ImportSceneParams>  (&import_scene);
    i.register_importer<ImportTextureParams>(&import_texture);
}


} // namespace josh
