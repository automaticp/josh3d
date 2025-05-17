#include "DefaultResources.hpp"
#include "ResourceInfo.hpp"
#include "ResourceRegistry.hpp"


namespace josh {


void register_default_resource_info(ResourceInfo& m) {
    m.register_resource_type<RT::Scene>();
    m.register_resource_type<RT::Material>();
    m.register_resource_type<RT::MeshDesc>();
    m.register_resource_type<RT::StaticMesh>();
    m.register_resource_type<RT::SkinnedMesh>();
    m.register_resource_type<RT::Texture>();
    m.register_resource_type<RT::Skeleton>();
    m.register_resource_type<RT::Animation>();
}


void register_default_resource_storage(ResourceRegistry& r) {
    r.initialize_storage_for<RT::Scene>();
    r.initialize_storage_for<RT::MeshDesc>();
    r.initialize_storage_for<RT::Material>();
    r.initialize_storage_for<RT::StaticMesh>();
    r.initialize_storage_for<RT::SkinnedMesh>();
    r.initialize_storage_for<RT::Texture>();
    r.initialize_storage_for<RT::Skeleton>();
    r.initialize_storage_for<RT::Animation>();
}


} // namespace josh
