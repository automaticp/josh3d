#include "Logging.hpp"
#include <iostream>


namespace josh {


std::ostream* logstream_{ &std::clog };


void set_logstream(std::ostream& os) { logstream_ = &os; }
auto logstream() -> std::ostream& { return *logstream_; }


/*
TODO:
- Add Imgui Demo to the imgui layer for testing stuff
- Do Overlays? How else to do logs?
- Use boost to make a tee?
*/


} // namespace josh
