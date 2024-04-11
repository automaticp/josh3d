#pragma once
#include "CommonConcepts.hpp" // IWYU pragma: keep
#include "Filesystem.hpp"
#include "VirtualFilesystemError.hpp"
#include <compare>




namespace josh {

namespace error {

class VirtualPathIsNotRelative final : public VirtualFilesystemError {
public:
    static constexpr auto prefix = "Virtual Path Is Not Relative: ";
    Path path;
    VirtualPathIsNotRelative(Path path)
        : VirtualFilesystemError(prefix, path)
        , path{ std::move(path) }
    {}
};


} // namespace error




/*
VPath (VirtualPath) is a wrapper around Path that represents path a that:

- Is relative to some real directory. Subsequently the VPath cannot be absolute.
- Is intended to be resolved to a real File or Directory through the VirtualFilesystem.

It is not, and does not have to be referring to an existing entry at the point of construction.
*/
class VPath {
private:
    Path vpath_;
    friend class VirtualFilesystem;

public:
    template<typename ...Args>
        requires not_move_or_copy_constructor_of<VPath, Args...>
    explicit VPath(Args&&... fs_path_args)
        : vpath_{ std::forward<Args>(fs_path_args)... }
    {
        if (vpath_.is_absolute()) {
            throw error::VirtualPathIsNotRelative(vpath_);
        }
    }

    const Path& path() const noexcept {
        return vpath_;
    }

    // Will decay to File through the thread_local VFS.
    operator File() const noexcept(false);

    // Will decay to Directory through the thread_local VFS.
    operator Directory() const noexcept(false);


    bool operator==(const VPath& other) const noexcept = default;
    std::strong_ordering operator<=>(const VPath& other) const noexcept = default;
};




namespace filesystem_literals {

inline VPath operator""_vpath(const char* str, size_t size) {
    return VPath{ str, str + size };
}

} // namespace filesystem_literals





} // namespace josh
