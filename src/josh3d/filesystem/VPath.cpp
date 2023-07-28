#include "VPath.hpp"
#include "VirtualFilesystem.hpp"


namespace josh {


VPath::operator File() const noexcept(false) {
    return vfs().resolve_file(*this);
}


VPath::operator Directory() const noexcept(false) {
    return vfs().resolve_directory(*this);
}


} // namespace josh
