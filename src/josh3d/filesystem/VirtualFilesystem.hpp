#pragma once
#include "CommonConcepts.hpp"
#include "Filesystem.hpp"
#include "VirtualFilesystemError.hpp"
#include "VFSRoots.hpp"
#include <utility>
#include <optional>

namespace josh {


class VPath;


namespace error {


class UnresolvedVirtualPath final : public VirtualFilesystemError {
public:
    static constexpr auto prefix = "Unresolved Virtual Path: ";
    Path path;
    UnresolvedVirtualPath(Path path)
        : VirtualFilesystemError(prefix, path)
        , path{ std::move(path) }
    {}
};


} // namespace error



// Point of access to a global (thread_local) VFS.
class VirtualFilesystem& vfs() noexcept;


/*
VirtualFilesystem (VFS) is an abstraction layer on top of the OS filesystem
that is primarily responsible for two things:

- Stores a list of root-directories ordered by priority.

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

Then if instead of trying to load from "./data/models/josh/josh.obj", we route
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
    VFSRoots roots_;

    // We could do some caching even, however...
    // Then the simple model of resolution becomes even more of a mess.
    // Maybe useful, but very fragile. Even a simple reorder in VFSRoots
    // invalidates the cache.

public:
    VirtualFilesystem() = default;

    VirtualFilesystem(VFSRoots root_dirs)
        : roots_{ std::move(root_dirs) }
    {}

    auto& roots() noexcept { return roots_; }
    const auto& roots() const noexcept { return roots_; }


    // Almost non-throwing file resolution*.
    // Matches VPath against the roots in contained order
    // until a valid file is found.
    //
    // * - Can throw underlying filesystem errors from is_valid()
    // checks. Does not throw library-level exceptions.
    [[nodiscard]] auto try_resolve_file(const VPath& vpath) const
        -> std::optional<File>;


    // Throwing file resolution. Matches VPath against the
    // roots in contained order until a valid file is found.
    //
    // Throws `error::UnresolvedVirtualPath` on failure.
    [[nodiscard]] auto resolve_file(const VPath& vpath) const
        -> File;


    // Almost non-throwing directory resolution*.
    // Matches VPath against the roots in contained order
    // until a valid directory is found.
    //
    // * - Can throw underlying filesystem errors from is_valid()
    // checks. Does not throw library-level exceptions.
    [[nodiscard]] auto try_resolve_directory(const VPath& vpath) const
        -> std::optional<Directory>;


    // Throwing directory resolution. Matches VPath against the
    // roots in contained order until a valid directory is found.
    //
    // Throws `error::UnresolvedVirtualPath` on failure.
    [[nodiscard]] auto resolve_directory(const VPath& vpath) const
        -> Directory;
};




} // namespace josh
