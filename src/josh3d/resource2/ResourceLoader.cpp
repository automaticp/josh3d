#include "ResourceLoader.hpp"
#include "ContainerUtils.hpp"
#include "ResourceInfo.hpp"


namespace josh {


auto ResourceLoader::_start_loading(const key_type& key, const UUID& uuid)
    -> Job<>
{
    if (auto* loader = try_find_value(dispatch_table_, key)) {
        // HMM: The loader actually returns a job here, but we do not track it
        // when we submit a load in the get_resource() call.
        //
        // The lifetime is fine, it will self-destroy once finished, but
        // aren't we missing out on something important here by discarding it?
        return (*loader)(ResourceLoaderContext(*this), uuid);
    } else {
        throw_fmt("No loader found for resource type {}.", resource_info().name_or_id(key));
    }
}


} // namespace josh
