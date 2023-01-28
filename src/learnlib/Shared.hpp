#pragma once
#include <memory>

namespace learn {

// FIXME: Make T const?
template<typename T>
using Shared = std::shared_ptr<T>;

} // namespace learn
