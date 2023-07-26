#pragma once
#include "CommonConcepts.hpp"
#include "Filesystem.hpp"
#include "RuntimeError.hpp"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <utility>
#include <optional>
#include <list>

namespace josh {


namespace error {


class VirtualFilesystemError : public RuntimeError {
public:
    static constexpr auto prefix = "Virtual Filesystem Error: ";
    VirtualFilesystemError(std::string msg)
        : VirtualFilesystemError(prefix, std::move(msg))
    {}
protected:
    VirtualFilesystemError(const char* prefix, std::string msg)
        : RuntimeError(prefix, std::move(msg))
    {}
};




class VirtualPathIsNotRelative final : public VirtualFilesystemError {
public:
    static constexpr auto prefix = "Virtual Path Is Not Relative: ";
    Path path;
    VirtualPathIsNotRelative(Path path)
        : VirtualFilesystemError(prefix, path)
        , path{ std::move(path) }
    {}
};


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




/*
The container of VFS that is responsible for storing and managing
insertion/removal of roots.

Behaves like a set based on the filesystem entry *equivalence*, not lexical equality of paths.

Iterators are only invalidated on removal and only for removed
elements.

Is ordered by push operations, with newly pushed elements inserted
at the front.
*/
class VFSRoots {
private:
    using list_type = std::list<Directory>;
    list_type roots_;

public:
    VFSRoots() = default;

    const list_type& list() const noexcept { return roots_; }

    auto begin()  const noexcept { return roots_.cbegin(); }
    auto cbegin() const noexcept { return roots_.cbegin(); }
    auto end()    const noexcept { return roots_.cend();   }
    auto cend()   const noexcept { return roots_.cend();   }


    // Push the Directory to the front of the list.
    // If the equivalent root already exists in the list,
    // return the iterator referring to it instead.
    // On succesful insertion return end() iterator.
    //
    // A shorthand for:
    //   insert_before(list().begin(), dir);
    auto push_front(const Directory& dir)
        -> list_type::const_iterator
    {
        return insert_before(begin(), dir);
    }


    auto push_front(Directory&& dir)
        -> list_type::const_iterator
    {
        return insert_before(begin(), std::move(dir));
    }


    // Insert the Directory before the element `pos`.
    // If the equivalent root already exists in the list,
    // return the iterator referring to it instead.
    // On succesful insertion return end() iterator.
    //
    // Yeah, returning end() on success is weird, but
    // this setup gives you the most information, while
    // keeping the interface concise.
    //
    // Decrement the `pos` if you need your newly
    // inserted element.
    auto insert_before(list_type::const_iterator pos,
        const Directory& dir)
        -> list_type::const_iterator
    {
        auto existing_it =
            find_equivalent_root(dir.path());

        if (existing_it == roots_.end()) {
            roots_.insert(pos, dir);
        }

        return existing_it;
    }


    auto insert_before(list_type::const_iterator pos,
        Directory&& dir)
        -> list_type::const_iterator
    {
        auto existing_it =
            find_equivalent_root(dir.path());

        if (existing_it == roots_.end()) {
            roots_.insert(pos, std::move(dir));
        }

        return existing_it;
    }


    // Reorder an element to be placed before another one.
    void order_before(
        list_type::const_iterator before_this_element,
        list_type::const_iterator element_to_reorder) noexcept
    {
        roots_.splice(before_this_element, roots_, element_to_reorder);
    }


    // Remove the root by equivalent path and return the
    // removed Directory entry.
    //
    // FIXME: Technically does not force you to construct
    // the Directory, thus avoiding the existence check,
    // but wouldn't the equivalence check then fail
    // anyways?
    auto remove(const Path& path) noexcept
        -> std::optional<Directory>
    {
        std::optional<Directory> removed_dir{ std::nullopt };

        auto it = find_equivalent_root(path);
        if (it != roots_.end()) {
            Directory dir{ std::move(*it) };
            roots_.erase(it);
            return { std::move(dir) };
        }

        return removed_dir;
    }


    // Behaves like list::erase, except returns a const_iterator
    // because you're not supposed to modify the elements.
    //
    // UB if `iter` does not refer to an element of VFSRoots.
    //
    // Returns iterator to the next element or the updated end()
    // iterator if the erased element was last.
    auto erase(list_type::const_iterator iter) noexcept
        -> list_type::const_iterator
    {
        return roots_.erase(iter);
    }


private:
    auto find_equivalent_root(const Path& path) noexcept
        -> list_type::iterator
    {
        return std::find_if(roots_.begin(), roots_.end(),
            [&](const Directory& root) {
                return std::filesystem::equivalent(root.path(), path);
            });
    }

    auto find_equivalent_root(const Path& path) const noexcept
        -> list_type::const_iterator
    {
        return std::find_if(roots_.begin(), roots_.end(),
            [&](const Directory& root) {
                return std::filesystem::equivalent(root.path(), path);
            });
    }

};





// Point of access to a global (thread_local) VFS.
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
    VFSRoots roots_;

    // We could do some caching even

public:
    VirtualFilesystem() = default;

    VirtualFilesystem(VFSRoots root_dirs)
        : roots_{ std::move(root_dirs) }
    {}

    auto& roots() noexcept { return roots_; }
    const auto& roots() const noexcept { return roots_; }


    // Part of the API that resolves paths to directory_entries
    [[nodiscard]]
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
    [[nodiscard]]
    auto resolve_file(const VPath& vpath) const
        -> File
    {
        if (auto maybe_file = try_resolve_file(vpath); maybe_file.has_value()) {
            return std::move(maybe_file.value());
        } else {
            throw error::UnresolvedVirtualPath(vpath.path());
        }
    }

    [[nodiscard]]
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

    [[nodiscard]]
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
