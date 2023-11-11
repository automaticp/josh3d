#include "TextureHelpers.hpp"
#include "Filesystem.hpp"
#include "MallocSupport.hpp"
#include "Pixels.hpp"
#include "ShaderSource.hpp"
#include "Size.hpp"
#include <stb_image.h>
#include <nlohmann/json.hpp>
#include <string>


namespace josh {
namespace detail {


template<typename ChanT>
ImageStorage<ChanT> load_image_from_file_impl(
    const File& file, size_t n_channels)
{
    stbi_set_flip_vertically_on_load(true);
    int width, height, n_channels_out;

    ChanT* data;
    if constexpr (std::same_as<ChanT, ubyte_t>) {
        data = stbi_load(
            file.path().c_str(),
            &width, &height, &n_channels_out,
            int(n_channels)
        );
    } else if constexpr (std::same_as<ChanT, float>) {
        data = stbi_loadf(
            file.path().c_str(),
            &width, &height, &n_channels_out,
            int(n_channels)
        );
    } else {
        static_assert(sizeof(ChanT) == 0);
    }

    if (!data) {
        throw error::ImageReadingError(
            file.path(), stbi_failure_reason()
        );
    }

    return {
        unique_malloc_ptr<ChanT[]>{ data },
        Size2S(width, height)
    };
}


template ImageStorage<ubyte_t> load_image_from_file_impl<ubyte_t>(
    const File &file, size_t n_channels);

template ImageStorage<float>   load_image_from_file_impl<float>(
    const File &file, size_t n_channels);


std::array<File, 6> parse_cubemap_json_for_files(const File& json_file) {
    // FIXME: read_file comes from the wrong place.
    std::string contents = read_file(json_file);

    Directory base_dir{ json_file.path().parent_path() };
    using json = nlohmann::json;
    json j = json::parse(contents);

    auto at = [&](const char* key) {
        return File(base_dir.path() / j.at(key).template get<std::string>());
    };

    return {
        at("posx"), at("negx"),
        at("posy"), at("negy"),
        at("posz"), at("negz")
    };
}


} // namespace detail
} // namespace josh
