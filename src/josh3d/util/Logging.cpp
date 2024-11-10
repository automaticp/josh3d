#include "Logging.hpp"
#include <iostream>


namespace josh {


std::ostream* logstream_{ &std::clog };


void set_logstream(std::ostream& os) { logstream_ = &os; }
auto logstream() -> std::ostream& { return *logstream_; }


} // namespace josh
