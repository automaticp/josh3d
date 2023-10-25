#pragma once
#include <iostream>


namespace josh {


namespace globals {
extern std::ostream& logstream;
} // namespace globals


void enable_glbinding_logger(std::ostream& os = globals::logstream);


} // namespace josh
