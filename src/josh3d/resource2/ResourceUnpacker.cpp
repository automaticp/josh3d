#include "ResourceUnpacker.hpp"
#include "ResourceInfo.hpp"
#include "Errors.hpp"


namespace josh {


auto ResourceUnpacker::_unpack(const key_type& key, UUID uuid, AnyRef destination)
    -> Job<>
{
    if (auto* unpacker = try_find_value(dispatch_table_, key))
    {
        // FIXME: Something about const-correctness is amiss here. Likely because of UniqueFunction.
        return (*unpacker)(ResourceUnpackerContext(*this), uuid, destination);
    }
    else
    {
        throw_fmt("No unpacker found for resource type {} and destination type {}.",
            resource_info().name_or_id(key.resource_type), key.destination_type.pretty_name());
    }
}


} // namespace josh
