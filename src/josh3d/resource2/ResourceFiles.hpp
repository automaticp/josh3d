#pragma once
#include "Common.hpp"
#include "Resource.hpp"
#include "Errors.hpp"
#include "Math.hpp"
#include "Scalars.hpp"
#include "UUID.hpp"


/*
Common vocabulary for resource files.
*/
namespace josh {

/*
Contents of a resource file make no sense.
*/
JOSH3D_DERIVE_EXCEPTION(InvalidResourceFile, RuntimeError);


template<usize N>
using FileTypeHS = FixedHashedString<N>;
using FileType   = HashedID;

/*
First bytes of each non-inline binary resource file.

ImHex Pattern:

struct Preamble
{
    char _magic[4];
    u32  file_type;
    u16  version;
    u16  _reserved;
    u32  resource_type;
    u8   self_uuid[16];
};
*/
struct alignas(8) ResourcePreamble
{
    u32          _magic;        // "josh".
    FileType     file_type;     // Type of the file.
    u16          version;       // Version of the file format.
    u16          _reserved0;    //
    ResourceType resource_type; // Type of stored resource.
    UUID         resource_uuid; // UUID of stored resource.

    static auto create(
        FileType     file_type,
        u16          version,
        ResourceType resource_type,
        const UUID&  resource_uuid) noexcept
            -> ResourcePreamble;
};

/*
A string type with fixed byte size for use in binary files.
Not guaranteed to be null-terminated.

ImHex Pattern:

struct ResourceName
{
    u8     len;
    char   name[len];
    padding[63 - len];
};
*/
struct ResourceName
{
    static constexpr usize max_length = 63;

    u8   length;
    char string[max_length];

    // Construct from a string view.
    // Truncates if the string is longer than `max_length`.
    static auto from_view(StrView sv) noexcept
        -> ResourceName;

    // Construct from a null-terminated string.
    // Truncates if the string is longer than `max_length`.
    static auto from_cstr(const char* cstr) noexcept
        -> ResourceName;

    auto view() const noexcept -> StrView;
    operator StrView() const noexcept { return view(); }
};


} // namespace josh
