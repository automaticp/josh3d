#pragma once
#include <iosfwd>


namespace josh {


void set_logstream(std::ostream& os);
auto logstream() -> std::ostream&;


} // namespace josh
