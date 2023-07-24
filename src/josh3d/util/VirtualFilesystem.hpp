#pragma once
#include "CommonConcepts.hpp"
#include "Filesystem.hpp"
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <optional>
#include <list>

namespace josh {


namespace error {

class VirtualFilesystemError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};


class VirtualPathIsNotRelative : public VirtualFilesystemError {
public:
    using VirtualFilesystemError::VirtualFilesystemError;
};


class UnresolvedVirtualPath : public VirtualFilesystemError {
public:
    using VirtualFilesystemError::VirtualFilesystemError;
};


} // namespace error



/*
VPath (VirtualPath) is a wrapper around std::filesystem::path that represents path a that:

- Is relative to some real directory. Subsequently the VPath cannot be absolute.
- Is intended to be resolved to a real File or Directory through the VirtualFilesystem.

It is not, and does not have to be referring to an existing entry at the point of construction.
*/
class VPath {
private:
    std::filesystem::path vpath_;
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

    const std::filesystem::path& path() const noexcept {
        return vpath_;
    }

    // Will decay to File through the thread_local VFS.
    operator File() const noexcept(false);

    // Will decay to Directory through the thread_local VFS.
    operator Directory() const noexcept(false);

};




namespace filesystem_literals {

inline VPath operator""_vpath(const char* str, size_t size) {
    return VPath{ str, str + size };
}

} // namespace filesystem_literals




// Point of access to a global (thread_local) VFS.
// FIXME: How to transfer roots?
class VirtualFilesystem& vfs() noexcept;


/*
VirtualFilesystem (VFS) is an abstraction layer on top of the OS filesystem
that is primarily responsible for two things:

- Stores a set of root-directories ordered by priority.

- Resolves *textual* paths specified as if relative to one of the root-directories
  to *real* directory entries. Validates that the entry actually exists.


Use cases:

Overall there are three kinds of interfaces that interact with concepts similar
to one of (path, file, directory) that I found existing currenty in code:

- from_file(...) which is a low level static constructor for some data type
  such as ShaderSource or TextureData. Should take just a File as it has no relation
  to any high-level organization of virtual paths, etc.

- Slightly higher-level loaders like ones in AssimpModelLoader, which should still
  refer to Files and not VirtualPaths. The VirtualPath component can be added
  in the calling "ResourceManager" or whatever acts as one. Plus the user can
  construct and pass the VPath which will implicitly resolve into File or Directory.

- Pools that store by path as if by unique ID. This might still be best stored
  by File. Again, it's the files that uniquely identify assets, not virtual paths.
  This layer is somewhat stuck between low-level loaders and a concept of high-level
  ResourceManagers, but without the latter it's hard to tell how exactly it
  should behave.

You might be asking: if every interface can get by with just File and Directory
what's the point of VFS then?

The VFS is a translation layer from a virtual path to a real filesystem entry,
where the virtual path is intended to be Client/User facing abstract representation.
Assume that I have a certain "ResourceManager", and want to load an asset represented
by its "location": "data/models/josh/josh.obj".

If the current working directory of the application is not a root that contains
"data/models/josh/josh.obj", then the attempt to construct a File object from it
or subsequently load the model will fail. However, assume that our VFS has at least
three roots in store right now:

[ "/home/user/", "/home/user/assets/", "./" ]

and the file "/home/user/assets/data/models/josh/josh.obj" exists and can be loaded.

Then if instead of trying to load from "data/models/josh/josh.obj"_file, we route
the path through the VFS (or most likely in the end, ResourceManager), then
the VFS layer will try matching
[
    "/home/user/data/models/josh/josh.obj",
    "/home/user/assets/data/models/josh/josh.obj",
    "./data/models/josh/josh.obj"
]
in that order and return the first match that corresponds to a real file.

Even right now, a simple ImGui loader widget can be routed through VFS to take advantage
of external asset loading and substitution.


WIP:

There are certain uncertainties in the implementation right now, mainly related to
the existence of a VFS instance in the presence of multiple threads. It's still
not clear whether a VFS instance should be global, thread_local, or belong to
a certain ResourceManager that handles it's lifetime in the separate thread
and coordinates communication with VFS from outside.

Currently, it's just a thread_local instance accessible from the vfs() function.
Once a ResourceManager is implemented, we'll rethink this.
*/
class VirtualFilesystem {
private:
    std::list<Directory> roots_;

    // We could do some caching even

public:
    VirtualFilesystem() = default;

    VirtualFilesystem(std::list<Directory> root_dirs)
        : roots_{ std::move(root_dirs) }
    {}


    // Part of the API that handles manipulation of the set of roots


    void push_root(Directory directory) {
        roots_.emplace_front(std::move(directory));
    }

    // ...


    // Part of the API that resolves paths to directory_entries

    auto try_resolve_file(const VPath& vpath) const
        -> std::optional<File>
    {

        for (auto&& root : roots_) {
            auto maybe_path = root.path() / vpath.path();
            auto maybe_file = File::try_make(maybe_path);

            if (maybe_file.has_value()) {
                return maybe_file;
            }
            // Failure here is not critical and we shouldn't throw,
            // we *try* different roots and only if nothing works
            // return nullopt.
        }

        return std::nullopt;
    }

    // Hmm, two error handling schemes...
    auto resolve_file(const VPath& vpath) const
        -> File
    {
        if (auto maybe_file = try_resolve_file(vpath); maybe_file.has_value()) {
            return std::move(maybe_file.value());
        } else {
            throw error::UnresolvedVirtualPath(vpath.path());
        }
    }


    auto try_resolve_directory(const VPath& vpath) const
        -> std::optional<Directory>
    {
        for (auto&& root : roots_) {
            auto maybe_path = root.path() / vpath.path();
            auto maybe_dir  = Directory::try_make(maybe_path);

            if (maybe_dir.has_value()) {
                return maybe_dir;
            }
        }

        return std::nullopt;
    }


    auto resolve_directory(const VPath& vpath) const
        -> Directory
    {
        if (auto maybe_dir = try_resolve_directory(vpath); maybe_dir.has_value()) {
            return std::move(maybe_dir.value());
        } else {
            throw error::UnresolvedVirtualPath(vpath.path());
        }
    }



    auto resolve_all()
        /* -> ??? */
    {

    }

};





inline VPath::operator File() const noexcept(false) {
    return vfs().resolve_file(*this);
}

inline VPath::operator Directory() const noexcept(false) {
    return vfs().resolve_directory(*this);
}






} // namespace josh
