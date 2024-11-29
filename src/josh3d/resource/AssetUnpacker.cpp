#include "AssetUnpacker.hpp"
#include "Active.hpp"
#include "Asset.hpp"
#include "CategoryCasts.hpp"
#include "ComponentLoaders.hpp"
#include "ECS.hpp"
#include "GLAPIBinding.hpp"
#include "Skybox.hpp"
#include <cstddef>
#include <type_traits>


namespace josh {


// Externally inacessible wrapper types used to create "private" storage in the registry.
template<typename T> struct Pending { T value; };
template<typename T> struct Retired { T value; };

using ModelJob  = SharedJob<SharedModelAsset>;
using SkyboxJob = SharedJob<SharedCubemapAsset>;


void AssetUnpacker::submit_model_for_unpacking(
    Entity                      entity,
    SharedJob<SharedModelAsset> model_job)
{
    registry_.emplace_or_replace<Pending<ModelJob>>(entity, MOVE(model_job));
}


void AssetUnpacker::submit_skybox_for_unpacking(
    Entity                        entity,
    SharedJob<SharedCubemapAsset> skybox_job)
{
    registry_.emplace_or_replace<Pending<SkyboxJob>>(entity, MOVE(skybox_job));
}


auto AssetUnpacker::num_pending() const noexcept
    -> size_t
{
    const size_t result =
        registry_.view<Pending<ModelJob>>().size() +
        registry_.view<Pending<SkyboxJob>>().size();
    // Add more pending if supported.
    return result;
}


void AssetUnpacker::wait_until_all_pending_are_complete() const {
    auto wait = [&]<typename T>(std::type_identity<T>) {
        for (const auto [e, pending] : registry_.view<Pending<T>>().each()) {
            pending.value.wait_until_ready();
        }
    };
    wait(std::type_identity<ModelJob> {});
    wait(std::type_identity<SkyboxJob>{});
}


auto AssetUnpacker::retire_completed_requests()
    -> size_t
{
    size_t num_retired{ 0 };

    auto retire = [&]<typename T>(std::type_identity<T>) {
        for (auto [e, pending] : registry_.view<Pending<T>>().each()) {
            registry_.emplace_or_replace<Retired<T>>(e, MOVE(pending.value));
            registry_.erase<Pending<T>>(e);
            ++num_retired;
        }
    };

    retire(std::type_identity<ModelJob> {});
    retire(std::type_identity<SkyboxJob>{});

    return num_retired;
}


auto AssetUnpacker::num_retired() const noexcept
    -> size_t
{
    const size_t result =
        registry_.view<Retired<ModelJob>> ().size() +
        registry_.view<Retired<SkyboxJob>>().size();
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

    for (const Entity entity : registry_.view<Retired<ModelJob>>()) {
        handle = { registry_, entity };
        ModelJob retired = MOVE(handle.get<Retired<ModelJob>>().value);
        handle.erase<Retired<ModelJob>>(); // Erase first, so that on exception the request is gone.
        SharedModelAsset asset = retired.get_result(); // Can rethrow the exception here.
        handle.emplace_or_replace<AssetPath>(asset.path);
        emplace_model_asset_into(handle, MOVE(asset));
        return;
    }

    for (const Entity entity : registry_.view<Retired<SkyboxJob>>()) {
        handle = { registry_, entity };
        SkyboxJob retired = MOVE(handle.get<Retired<SkyboxJob>>().value);
        handle.erase<Retired<SkyboxJob>>();
        SharedCubemapAsset asset = retired.get_result(); // Can rethrow the exception here.
        handle.emplace_or_replace<AssetPath>(MOVE(asset.path));
        make_available<Binding::Cubemap>(asset.cubemap->id());
        handle.emplace_or_replace<Skybox>(MOVE(asset.cubemap));

        // TODO: Is this the right place to handle this?
        if (!has_active<Skybox>(registry_)) {
            make_active<Skybox>(handle);
        }
        return;
    }
}




} // namespace josh
