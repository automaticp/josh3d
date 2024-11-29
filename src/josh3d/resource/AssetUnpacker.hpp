#pragma once
#include "Asset.hpp"
#include "Coroutines.hpp"
#include "ECS.hpp"


namespace josh {


/*
Keeps a list of pending load requests, and unpacks loaded assets into the scene registry.

Each request can exist in one of the 3 states:

    - Incomplete Pending - made complete asynchronously;
    - Complete Pending   - must be retired with a call to `retire_completed_requests()`;
    - Retired            - must be unpacked with a call to `unpack_one_request()`.

*/
class AssetUnpacker {
public:
    AssetUnpacker(Registry& registry) : registry_{ registry } {}

    // Associate an entity with a pending state for a model.
    void submit_model_for_unpacking(Entity entity, SharedJob<SharedModelAsset> model_job);

    // Associate an entity with a pending state for a skybox.
    void submit_skybox_for_unpacking(Entity entity, SharedJob<SharedCubemapAsset> skybox_job);

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
    //     unpacker.retire_completed_requests();
    //     while (unpacker.can_unpack_more()) {
    //         try {
    //             unpacker.unpack_one_retired();
    //         } catch (const RuntimeError& e) {
    //             // Log or do something else.
    //         }
    //     }
    //
    // Returns a handle associated with the new imported asset
    // or an "invalid handle" if nothing was unpacked.
    //
    auto unpack_one_retired() -> Handle;

    // An overload of `unpack_one_retired()` that will set the `out_handle`
    // before attempting actions that could result in an exception.
    //
    // This allows you to know unpacking which handle failed during
    // exception handling.
    void unpack_one_retired(Handle& out_handle);

private:
    Registry& registry_;
};



} // namespace josh
