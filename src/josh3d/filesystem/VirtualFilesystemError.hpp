#pragma once
#include "Errors.hpp"


namespace josh {

/*
NOTE: It is in a separate header not because it's important or anything,
it's just that multiple things depend on it.
*/
JOSH3D_DERIVE_EXCEPTION(VirtualFilesystemError, RuntimeError);

} // namespace josh

