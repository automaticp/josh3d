#include "UUID.hpp"
#include "Common.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


namespace josh {


auto generate_uuid() noexcept
    -> UUID
{
    // NOTE: We use mt19937 backed generator and not the "pure" one, since this one cannot fail
    // during generation, only during initialization. Although I have no idea why getrandom()
    // would ever fail at all.
    //
    // NOTE: Boost seeds the generator on default construction from a random_device()-equivalent
    // source. No need to seed our own mt19937 here.
    //
    // NOLINTNEXTLINE(misc-use-internal-linkage): Are you ok? This is storage duration, not linkage.
    thread_local boost::uuids::random_generator_mt19937 random_generator;
    return random_generator();
}

auto deserialize_uuid(StrView string_repr)
    -> UUID
{
    // NOLINTNEXTLINE(misc-use-internal-linkage)
    thread_local boost::uuids::string_generator string_generator;
    return string_generator(string_repr.begin(), string_repr.end());
}

void serialize_uuid_to(std::span<char, 36> out_buf, const UUID& uuid)
{
    const bool success =
        boost::uuids::to_chars(uuid, out_buf.data(), out_buf.data() + out_buf.size());
    assert(success);
}

void serialize_uuid_to(std::span<char, 37> out_buf, const UUID& uuid)
{
    const bool success =
        boost::uuids::to_chars(uuid, out_buf.data(), out_buf.data() + out_buf.size());
    assert(success);
    out_buf[out_buf.size() - 1] = '\0';
}

auto serialize_uuid(const UUID& uuid)
    -> String
{
    return boost::uuids::to_string(uuid);
}


} // namespace josh
