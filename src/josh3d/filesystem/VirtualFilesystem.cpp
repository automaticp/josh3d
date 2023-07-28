#include "Filesystem.hpp"
#include "VirtualFilesystem.hpp"
#include "VPath.hpp"
#include <functional>
#include <optional>
#include <utility>


using namespace josh::filesystem_literals;

namespace josh {


VirtualFilesystem& vfs() noexcept {
    thread_local VirtualFilesystem vfs{
        std::invoke([]() -> VFSRoots {
            VFSRoots roots;
            roots.push_front("./"_directory);
            return roots;
        })
    };
    return vfs;
}




auto VirtualFilesystem::try_resolve_file(const VPath& vpath) const
    -> std::optional<File>
{

    for (auto&& root : roots_) {
        // With the current design as is, having invalid roots
        // during resolution does not invalidate the result
        // of that resolution.
        //
        // So we don't bother to check.

        auto maybe_path = root.path() / vpath.path();
        auto maybe_file = File::try_make(maybe_path);


        if (maybe_file.has_value()) {
            return maybe_file;
        }
        // Failure to match a file is not critical and
        // we shouldn't throw, we *try* different roots
        // and only if nothing works return nullopt.
    }

    return std::nullopt;
}


auto VirtualFilesystem::resolve_file(const VPath& vpath) const
    -> File
{
    if (auto maybe_file = try_resolve_file(vpath)) {
        return std::move(maybe_file.value());
    } else {
        throw error::UnresolvedVirtualPath(vpath.path());
    }
}


auto VirtualFilesystem::try_resolve_directory(const VPath& vpath) const
    -> std::optional<Directory>
{

    for (auto&& root : roots_) {
        auto maybe_path = root.path() / vpath.path();
        auto maybe_dir = Directory::try_make(maybe_path);

        if (maybe_dir.has_value()) {
            return maybe_dir;
        }
    }

    return std::nullopt;
}


auto VirtualFilesystem::resolve_directory(const VPath& vpath) const
    -> Directory
{
    if (auto maybe_dir = try_resolve_directory(vpath)) {
        return std::move(maybe_dir.value());
    } else {
        throw error::UnresolvedVirtualPath(vpath.path());
    }
}


} // namespace josh
