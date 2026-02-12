#include "ReadFile.hpp"
#include "Common.hpp"
#include <fstream>


namespace josh {

auto read_file(const File& file)
    -> String
{
    std::ifstream ifs{ file.path() };

    if (ifs.fail())
        throw FileReadingError(file.path());

    return String{
        std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>()
    };
}


} // namespace josh
