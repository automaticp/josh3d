#pragma once
#include <memory>

namespace josh {


// Type alias to make the implementation swappable later.
template<typename T>
using Shared = std::shared_ptr<T>;


template<typename T, typename ...Args>
auto make_shared(Args&&... args)
    -> Shared<T>
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}


} // namespace josh
