#pragma once
#include <memory>

namespace josh {

// FIXME: Make T const?
template<typename T>
using Shared = std::shared_ptr<T>;

} // namespace josh
