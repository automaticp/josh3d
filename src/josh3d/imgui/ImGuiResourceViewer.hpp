#pragma once
#include "AnimationStorage.hpp"
#include "AssetImporter.hpp"
#include "AsyncCradle.hpp"
#include "Common.hpp"
#include "Resource.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceUnpacker.hpp"
#include "ECS.hpp"
#include "MeshRegistry.hpp"
#include "SkeletonStorage.hpp"
#include "UIContextFwd.hpp"
#include "UniqueFunction.hpp"


namespace josh {

struct ResourceInspectorContext;

struct ImGuiResourceViewer
{
    void display(UIContext& ui);

    // TODO: Probably support multiple tabs. For now we use a popup.
    // void display_inspectors();

    // TODO: Importers.
    // template<typename T>
    // auto register_importer(ResourceType type) -> bool;

    template<typename T>
    auto register_inspector(ResourceType type) -> bool;

    using inspector_type         = UniqueFunction<void()>;
    using inspector_factory_type = UniqueFunction<inspector_type(UIContext&, UUID)>;
    HashMap<ResourceType, inspector_factory_type> _inspector_factories;
};


template<typename T>
auto ImGuiResourceViewer::register_inspector(ResourceType type)
    -> bool
{
    auto [it, was_emplaced] =
        _inspector_factories.try_emplace(type, [](UIContext& context, UUID uuid)
            -> inspector_type
        {
            return inspector_type(T(context, uuid));
        });
    return was_emplaced;
}


} // namespace josh
