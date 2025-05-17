#include "ReadFile.hpp"
#include <fstream>
#include <string>


namespace josh {


auto read_file(const File& file)
    -> std::string
{
    std::ifstream ifs{ file.path() };
    if (ifs.fail()) {
        throw error::FileReadingError(file.path());
    }

    return std::string{
        std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()
    };
}


} // namespace josh
