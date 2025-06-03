#pragma once
#include "AssetImporter.hpp"
#include "AsyncCradle.hpp"
#include "Common.hpp"
#include "Resource.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceUnpacker.hpp"
#include "ECS.hpp"
#include "MeshRegistry.hpp"
#include "UniqueFunction.hpp"


namespace josh {


struct ResourceInspectorContext;


struct ImGuiResourceViewer {
    ResourceDatabase&   resource_database;
    AssetImporter&      asset_importer;
    ResourceUnpacker&   resource_unpacker;
    Registry&           registry;
    const MeshRegistry& mesh_registry;
    AsyncCradleRef      async_cradle;

    void display_viewer();

    // TODO: Probably support multiple tabs. For now we use a popup.
    // void display_inspectors();

    // TODO: Importers.
    // template<typename T>
    // auto register_importer(ResourceType type) -> bool;

    template<typename T>
    auto register_inspector(ResourceType type) -> bool;

    using inspector_type         = UniqueFunction<void()>;
    using inspector_factory_type = UniqueFunction<inspector_type(ResourceInspectorContext, UUID)>;
    HashMap<ResourceType, inspector_factory_type> _inspector_factories;
};


// TODO: Surely more is needed.
struct ResourceInspectorContext {
    auto& resource_database() const noexcept { return _self.resource_database; }

    ImGuiResourceViewer& _self;
};


template<typename T>
auto ImGuiResourceViewer::register_inspector(ResourceType type)
    -> bool
{
    auto [it, was_emplaced] =
        _inspector_factories.try_emplace(type, [](ResourceInspectorContext context, UUID uuid)
            -> inspector_type
        {
            return inspector_type(T(MOVE(context), uuid));
        });
    return was_emplaced;
}


} // namespace josh
