#include "VirtualFilesystem.hpp"
#include <functional>


using namespace josh::filesystem_literals;

namespace josh {

VirtualFilesystem& vfs() noexcept {
    thread_local VirtualFilesystem vfs{
        std::invoke([]() -> VFSRoots {
            VFSRoots roots;
            roots.push_front(Directory("./"));
            return roots;
        })
    };
    return vfs;
}

} // namespace josh
