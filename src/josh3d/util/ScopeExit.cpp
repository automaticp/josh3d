#include "ScopeExit.hpp"
#include <exception>


namespace josh::detail {


auto uncaught_exceptions() noexcept -> int
{
    return std::uncaught_exceptions();
}


} // namespace josh::detail
