#include "AssetUnpacker.hpp"
#include "Active.hpp"
#include "Asset.hpp"
#include "CategoryCasts.hpp"
#include "ComponentLoaders.hpp"
#include "ECS.hpp"
#include <cstddef>


namespace josh {


// Externally inacessible wrapper types used to create "private" storage in the registry.
template<typename T> struct Pending { T value; };
template<typename T> struct Retired { T value; };


using PendingModel = Pending<SharedJob<SharedModelAsset>>;
using RetiredModel = Retired<SharedJob<SharedModelAsset>>; // Still a job, to preserve the exception until unpacking.


struct RetiredSkybox { AssetPath path; };


void AssetUnpacker::submit_model_for_unpacking(
    Entity                      entity,
    SharedJob<SharedModelAsset> model_job)
{
    using pending_type = Pending<SharedJob<SharedModelAsset>>;
    registry_.emplace_or_replace<pending_type>(entity, MOVE(model_job));
}


void AssetUnpacker::submit_skybox_for_unpacking(
    Entity    entity,
    AssetPath path)
{
    // This is special because we directly emplace retired state.
    // The loading will be done sychronously once the unpack() is called.
    registry_.emplace_or_replace<RetiredSkybox>(entity, MOVE(path));
}


auto AssetUnpacker::num_pending() const noexcept
    -> size_t
{
    const size_t result = registry_.view<PendingModel>().size();
    // Add more pending if supported.
    return result;
}


void AssetUnpacker::wait_until_all_pending_are_complete() const {
    for (const auto [e, pending] : registry_.view<PendingModel>().each()) {
        pending.value.wait_until_ready();
    }
}


auto AssetUnpacker::retire_completed_requests()
    -> size_t
{
    size_t num_retired{ 0 };
    for (auto [e, pending] : registry_.view<PendingModel>().each()) {
        if (pending.value.is_ready()) {
            registry_.emplace_or_replace<RetiredModel>(e, MOVE(pending.value));
            registry_.erase<PendingModel>(e);
            ++num_retired;
        }
    }
    return num_retired;
}


auto AssetUnpacker::num_retired() const noexcept
    -> size_t
{
    const size_t result =
        registry_.view<RetiredModel> ().size() +
        registry_.view<RetiredSkybox>().size();
    return result;
}


bool AssetUnpacker::can_unpack_more() const noexcept {
    return bool(num_retired());
}


auto AssetUnpacker::unpack_one_retired()
    -> Handle
{
    Handle handle;
    unpack_one_retired(handle);
    return handle;
}


void AssetUnpacker::unpack_one_retired(Handle& handle) {
    // For-loops are decieving here, we return after the first entity.
    handle = {};

    for (const Entity entity : registry_.view<RetiredModel>()) {
        handle = { registry_, entity };
        SharedJob<SharedModelAsset> retired = MOVE(handle.get<RetiredModel>().value);
        handle.erase<RetiredModel>(); // Erase first, so that on exception the request is gone.
        SharedModelAsset asset = retired.get_result(); // Can rethrow the exception here.
        handle.emplace_or_replace<AssetPath>(asset.path);
        emplace_model_asset_into(handle, MOVE(asset));
        return;
    }

    for (const Entity entity : registry_.view<RetiredSkybox>()) {
        handle = { registry_, entity };
        AssetPath apath = MOVE(handle.get<RetiredSkybox>().path);
        handle.erase<RetiredSkybox>();
        load_skybox_into(handle, File(apath.entry())); // Can throw here.
        handle.emplace_or_replace<AssetPath>(MOVE(apath));
        // TODO: Is this the right place to handle this?
        if (!has_active<Skybox>(registry_)) {
            make_active<Skybox>(handle);
        }
        return;
    }
}




} // namespace josh
