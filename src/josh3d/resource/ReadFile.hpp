#pragma once
#include "Common.hpp"
#include "Errors.hpp"
#include "Filesystem.hpp"


/*
Wow, what an amazing header...
*/
namespace josh {

JOSH3D_DERIVE_EXCEPTION(FileReadingError, RuntimeError);

auto read_file(const File& file) -> String;

} // namespace josh
