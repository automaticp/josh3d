#pragma once
#include "Common.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <span>


namespace josh {

using UUID = boost::uuids::uuid;

/*
Version 4, fully random.

TODO: Is version 7 with timestamps potentially more useful?
 */
auto generate_uuid() noexcept
    -> UUID;

auto deserialize_uuid(StrView string_repr)
    -> UUID;

/*
Writes an exact 36 char representation of a UUID
*/
void serialize_uuid_to(std::span<char, 36> out_buf, const UUID& uuid);

/*
Writes an exact 36 char representation of a UUID, Null-terminator is written as last char.
*/
void serialize_uuid_to(std::span<char, 37> out_buf, const UUID& uuid);

auto serialize_uuid(const UUID& uuid)
    -> String;


} // namespace josh
