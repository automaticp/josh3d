#pragma once
#include "RuntimeError.hpp"


/*
It is in a separate header not because it's important or anything,
it's just that multiple things depend on it.
*/
namespace josh::error {


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


} // namespace josh::error
