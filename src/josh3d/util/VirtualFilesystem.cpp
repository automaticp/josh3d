#include "VirtualFilesystem.hpp"


using namespace josh::filesystem_literals;

namespace josh {

VirtualFilesystem& vfs() noexcept {
    thread_local VirtualFilesystem vfs({ "./"_directory });
    return vfs;
}

} // namespace josh
