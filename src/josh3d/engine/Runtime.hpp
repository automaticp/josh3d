#pragma once
#include "AnimationStorage.hpp"
#include "AssetImporter.hpp"
#include "AssetManager.hpp"
#include "AssetUnpacker.hpp"
#include "AsyncCradle.hpp"
#include "ECS.hpp"
#include "MeshRegistry.hpp"
#include "Primitives.hpp"
#include "RenderEngine.hpp"
#include "ResourceDatabase.hpp"
#include "ResourceLoader.hpp"
#include "ResourceRegistry.hpp"
#include "ResourceUnpacker.hpp"
#include "SceneImporter.hpp"
#include "SkeletonStorage.hpp"


namespace glfw { class Window; }


namespace josh {


struct RuntimeParams
{
    glfw::Window& main_window;
    Path          database_root;
    usize         task_pool_size;
    usize         loading_pool_size;
    Extent2I      main_resolution;
    HDRFormat     main_format;
};

/*
A collection of systems and contexts that represent the core part of *the engine*.
It is useful to aggregate this and not pass around all pieces individually.

NOTE: The order of members here is not arbitrary. These members can depend on one
another. Reordering might lead to locking or segfaults on shutdown.
*/
struct Runtime
{
    explicit Runtime(const RuntimeParams& p);

    // Primary async contexts used by the engine. Thread pools,
    // offscreen GPU context, "local" main thread context, etc.
    AsyncCradle      async_cradle;

    // Various storage for loaded resources.
    MeshRegistry     mesh_registry;
    SkeletonStorage  skeleton_storage;
    AnimationStorage animation_storage;

    // Old asset system. TODO: Deprecate.
    AssetManager     asset_manager;
    AssetUnpacker    asset_unpacker;
    SceneImporter    scene_importer;

    // Boxes, spheres, quads, etc.
    Primitives       primitives;

    // Resource bookkeeping.
    ResourceDatabase resource_database;
    ResourceRegistry resource_registry;

    // New asset and resource processing.
    AssetImporter    asset_importer;
    ResourceLoader   resource_loader;
    ResourceUnpacker resource_unpacker;

    // The primary registry used as a main scene representation.
    Registry         registry;

    // A collection of rendering stages with some extra fluff on top.
    RenderEngine     renderer;
};


} // namespace josh
