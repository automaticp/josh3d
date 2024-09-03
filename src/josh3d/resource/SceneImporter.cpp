#include "SceneImporter.hpp"
#include "AssetManager.hpp"
#include "ComponentLoaders.hpp"
#include "Transform.hpp"
#include <algorithm>
#include <cstddef>
#include <entt/entity/fwd.hpp>
#include <iterator>


namespace josh {


void SceneImporter::request_model_import(const AssetPath& path) {
    pending_models_.push_back(assmanager_.load_model(path));
}


void SceneImporter::request_skybox_import(const AssetPath& path) {
    retired_skyboxes_.push_back(path);
}


void SceneImporter::wait_until_all_pending_are_complete() const {
    for (const auto& future : pending_models_) {
        future.wait_for_result();
    }
}


auto SceneImporter::num_pending() const noexcept
    -> size_t
{
    return pending_models_.size();
}


auto SceneImporter::retire_completed_requests()
    -> size_t
{
    auto subrange_of_completed =
        // This is likely very scuffed, because is_available() can become true as the sequence is
        // iterated. But it only changes one way. Either way, this is not to guaranteed
        // to be std::is_partitioned() after this operation. This could be UB...
        std::ranges::partition(pending_models_,
            // Partitions such that `true` are first, `false` are second,
            // and returns the range of the second group, that is: is_available() == true.
            [](const Future<SharedModelAsset>& future) { return !future.is_available(); });

    std::ranges::move(subrange_of_completed, std::back_inserter(retired_models_));

    const size_t num_retired = subrange_of_completed.size();

    pending_models_.erase(subrange_of_completed.begin(), subrange_of_completed.end());

    return num_retired;
}


auto SceneImporter::num_retired() const noexcept
    -> size_t
{
    return retired_models_.size() + retired_skyboxes_.size();
}


bool SceneImporter::can_unpack_more() const noexcept {
    return bool(num_retired());
}




namespace {


auto unpack_model(entt::registry& registry, Future<SharedModelAsset> future)
    -> entt::entity
{
    const entt::entity new_entity = registry.create();
    const entt::handle new_handle{ registry, new_entity };

    try {
        SharedModelAsset asset = get_result(std::move(future));
        new_handle.emplace<Transform>();
        new_handle.emplace<Name>(asset.path.file.filename());
        new_handle.emplace<Path>(canonical(asset.path.file));
        emplace_model_asset_into(new_handle, std::move(asset));
        return new_entity;
    } catch (...) {
        registry.destroy(new_entity);
        throw;
    }
}


auto unpack_skybox(entt::registry& registry, const AssetPath& json_file)
    -> entt::entity
{
    const entt::entity new_entity = registry.create();

    try {
        load_skybox_into({ registry, new_entity }, File(json_file.file));
        return new_entity;
    } catch (...) {
        registry.destroy(new_entity);
        throw;
    }
}


} // namespace




auto SceneImporter::unpack_one_retired()
    -> entt::entity
{

    if (!retired_models_.empty()) {

        Future<SharedModelAsset> future = std::move(retired_models_.back());
        retired_models_.pop_back(); // Make sure to remove this request first.

        return unpack_model(registry_, std::move(future));

    } else if (!retired_skyboxes_.empty()) {

        AssetPath path = retired_skyboxes_.back();
        retired_skyboxes_.pop_back(); // Make sure to remove this request first.

        return unpack_skybox(registry_, path);
    }

    return entt::null;
}


void SceneImporter::unpack_all_retired() {
    while (can_unpack_more()) { unpack_one_retired(); }
}




} // namespace josh
