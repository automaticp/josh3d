#pragma once
#include "CategoryCasts.hpp"
#include "CommonConcepts.hpp"
#include "Errors.hpp"
#include "Filesystem.hpp"
#include "VirtualFilesystemError.hpp"
#include <compare>


namespace josh {

JOSH3D_DERIVE_EXCEPTION_EX(VirtualPathIsNotRelative, VirtualFilesystemError, { Path path; });

/*
VPath (VirtualPath) is a wrapper around Path that represents path a that:

- Is relative to some real directory. Subsequently the VPath cannot be absolute.
- Is intended to be resolved to a real File or Directory through the VirtualFilesystem.

It is not, and does not have to be referring to an existing entry at the point of construction.
*/
class VPath
{
public:
    template<typename ...Args>
        requires not_move_or_copy_constructor_of<VPath, Args...>
    explicit VPath(Args&&... fs_path_args)
        : vpath_{ FORWARD(fs_path_args)... }
    {
        if (vpath_.is_absolute())
            throw VirtualPathIsNotRelative({}, { vpath_ });
    }

    auto path() const noexcept -> const Path& { return vpath_; }

    // Will decay to File through the thread_local VFS.
    operator File() const noexcept(false);

    // Will decay to Directory through the thread_local VFS.
    operator Directory() const noexcept(false);

    bool operator==(const VPath& other) const noexcept = default;
    std::strong_ordering operator<=>(const VPath& other) const noexcept = default;

private:
    Path vpath_;
    friend class VirtualFilesystem;
};


namespace filesystem_literals {

inline auto operator""_vpath(const char* str, size_t size)
    -> VPath
{
    return VPath{ str, str + size };
}

} // namespace filesystem_literals
} // namespace josh
