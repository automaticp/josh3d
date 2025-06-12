#include "ResourceFiles.hpp"
#include <algorithm>


namespace josh {


auto ResourcePreamble::create(
    FileType     file_type,
    u16          version,
    ResourceType resource_type,
    const UUID&  resource_uuid) noexcept
        -> ResourcePreamble
{
    const char magic[4] = { 'j', 'o', 's', 'h' };
    return {
        ._magic        = std::bit_cast<u32>(magic),
        .file_type     = file_type,
        .version       = version,
        ._reserved0    = {},
        .resource_type = resource_type,
        .resource_uuid = resource_uuid,
    };
}

auto ResourceName::from_view(StrView sv) noexcept
    -> ResourceName
{
    const usize length = std::min(max_length, sv.length());
    ResourceName result = {
        .length = uint8_t(length),
        .string = {},
    };
    std::ranges::copy(sv.substr(0, length), result.string);
    return result;
}

auto ResourceName::from_cstr(const char* cstr) noexcept
    -> ResourceName
{
    usize       space  = max_length;
    const char* cur    = cstr;
    while (space and *cur != '\0')
    {
        --space;
        ++cur;
    }
    const usize length = cur - cstr;
    ResourceName result = {
        .length = u8(length),
        .string = {},
    };
    std::ranges::copy(std::ranges::subrange(cstr, cur), result.string);
    return result;
}

auto ResourceName::view() const noexcept
    -> StrView
{
    return { string, std::min<usize>(length, max_length) };
}


} // namespace josh
