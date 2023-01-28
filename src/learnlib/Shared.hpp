#pragma once
#include <memory>

// FIXME: Make T const?
template<typename T>
using Shared = std::shared_ptr<T>;
