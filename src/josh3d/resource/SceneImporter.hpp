#pragma once
#include "AssetManager.hpp"
#include "Future.hpp"
#include <entt/entity/fwd.hpp>
#include <vector>


namespace josh {


/*
Keeps a list of pending load requests, and unpacks loaded assets into the scene registry.

Each request can exist in one of the 3 states:

    - Incomplete Pending - made complete asynchronously;
    - Complete Pending   - must be retired with a call to `retire_completed_requests()`;
    - Retired            - must be unpacked with a call to `unpack_one_request()`.

*/
class SceneImporter {
public:
    SceneImporter(AssetManager& assmanager, entt::registry& registry)
        : assmanager_{ assmanager }
        , registry_  { registry   }
    {}

    void request_model_import(const AssetPath& path);

    // NOTE: Not async right now. Will load when unpack is called.
    void request_skybox_import(const AssetPath& path);

    // Number of requests not yet retired. Either because they are
    // not complete, or because the `retire_completed_requests()`
    // hasn't been called for them.
    auto num_pending() const noexcept -> size_t;

    // This is a hard sync that blocks until all pending requests become
    // complete, and can be retired.
    void wait_until_all_pending_are_complete() const;

    // Moves the requests that have been completed from the
    // pending list, to the "retired" list, allowing
    // them to be unpacked.
    //
    // Returns the number of retired requests in this call.
    auto retire_completed_requests() -> size_t;

    // Number of completed requests that that are ready to be unpacked.
    auto num_retired() const noexcept -> size_t;

    // True if the importer can unpack more assets *right now*.
    // That is, it has some *completed* requests.
    //
    // New completed requests do not appear without calling
    // `retire_completed_requests()`.
    bool can_unpack_more() const noexcept;

    // This *must* be called after `retire_completed_requests()`.
    // Call this periodically to unpack completed requests into the registry.
    //
    // If one of the requests resulted in an exception, this will throw it, interrupting
    // the resolution process. Keep calling this function in a loop to "handle"
    // all exceptions.
    //
    //     importer.retire_completed_requests();
    //     while (importer.can_unpack_more()) {
    //         try {
    //             importer.unpack_one_request();
    //         } catch (const RuntimeError& e) {
    //             // Log or do something else.
    //         }
    //     }
    //
    // Returns an entity associated with the new imported asset
    // or `entt::null` if nothing was unpacked.
    //
    auto unpack_one_retired() -> entt::entity;

    // This *must* be called after `retire_completed_requests()`.
    // Call this periodically to unpack completed requests into the registry.
    //
    // Equivalent to:
    //
    //     while (importer.can_unpack_more()) {
    //         importer.unpack_one_retired();
    //     }
    //
    // If unpacking throws an exception, the process will be interrupted,
    // and the exception will be propagated to the caller.
    //
    void unpack_all_retired();

private:
    AssetManager&   assmanager_;
    entt::registry& registry_;

    std::vector<Future<SharedModelAsset>> pending_models_;
    std::vector<Future<SharedModelAsset>> retired_models_;

    std::vector<AssetPath> retired_skyboxes_; // Not async, so they are never "pending".
};


} // namespace josh
