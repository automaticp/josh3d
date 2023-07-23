#include "TextureData.hpp"
#include "Filesystem.hpp"
#include <stb_image.h>
#include <stdexcept>


namespace josh {


TextureData TextureData::from_file(const File& file,
    bool flip_vertically, int num_desired_channels) noexcept(false)
{
    stbi_set_flip_vertically_on_load(flip_vertically);

    int width, height, n_channels;
    unsigned char* data =
        stbi_load(file.path().c_str(), &width, &height, &n_channels, num_desired_channels);

    if (!data) {
        // FIXME: Replace with custom error type.
        throw std::runtime_error(
            "Stb could not load the image at " + file.path().native() +
            ". Reason: " + stbi_failure_reason()
        );
    }

    return TextureData(
        std::unique_ptr<unsigned char[], FreeDeleter>{ data },
        width, height, n_channels
    );
}


} // namespace josh
