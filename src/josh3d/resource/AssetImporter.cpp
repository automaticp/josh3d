#include "AssetImporter.hpp"
#include "Active.hpp"
#include "AssetLoader.hpp"
#include "ComponentLoaders.hpp"
#include "ContainerUtils.hpp"
#include <entt/entity/fwd.hpp>
#include <algorithm>
#include <cstddef>
#include <iterator>


namespace josh {


void AssetImporter::request_model_import(const AssetPath& path, entt::handle handle) {
    pending_models_.push_back({ asset_loader_.load_model(path), handle });
}


void AssetImporter::request_skybox_import(const AssetPath& path, entt::handle handle) {
    retired_skyboxes_.push_back({ path, handle });
}


void AssetImporter::wait_until_all_pending_are_complete() const {
    for (const auto& request : pending_models_) {
        request.future.wait_for_result();
    }
}


auto AssetImporter::num_pending() const noexcept
    -> size_t
{
    return pending_models_.size();
}


auto AssetImporter::retire_completed_requests()
    -> size_t
{
    auto subrange_of_completed =
        // This is likely very scuffed, because is_available() can become true as the sequence is
        // iterated. But it only changes one way. Either way, this is not to guaranteed
        // to be std::is_partitioned() after this operation. This could be UB...
        std::ranges::partition(pending_models_,
            // Partitions such that `true` are first, `false` are second,
            // and returns the range of the second group, that is: is_available() == true.
            [](const ModelRequest& request) { return !request.future.is_available(); });

    std::ranges::move(subrange_of_completed, std::back_inserter(retired_models_));

    const size_t num_retired = subrange_of_completed.size();

    pending_models_.erase(subrange_of_completed.begin(), subrange_of_completed.end());

    return num_retired;
}


auto AssetImporter::num_retired() const noexcept
    -> size_t
{
    return retired_models_.size() + retired_skyboxes_.size();
}


bool AssetImporter::can_unpack_more() const noexcept {
    return bool(num_retired());
}




auto AssetImporter::unpack_one_retired()
    -> entt::handle
{

    if (!retired_models_.empty()) {

        ModelRequest     request = pop_back(retired_models_);
        SharedModelAsset asset   = get_result(std::move(request.future)); // Can throw.
        request.handle.emplace_or_replace<AssetPath>(asset.path);
        emplace_model_asset_into(request.handle, std::move(asset));
        return request.handle;

    } else if (!retired_skyboxes_.empty()) {

        SkyboxRequest request = pop_back(retired_skyboxes_);
        request.handle.emplace_or_replace<AssetPath>(request.path);
        load_skybox_into(request.handle, File(request.path.file));
        // TODO: Is this the right place to handle this?
        if (!has_active<Skybox>(*request.handle.registry())) {
            make_active<Skybox>(request.handle);
        }
        return request.handle;
    }

    return {};
}




} // namespace josh
