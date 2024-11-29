#include "TextureHelpers.hpp"
#include "Channels.hpp"
#include "Filesystem.hpp"
#include "MallocSupport.hpp"
#include "ReadFile.hpp"
#include "Size.hpp"
#include <algorithm>
#include <stb_image.h>
#include <nlohmann/json.hpp>
#include <string>


namespace josh {
namespace detail {


template<typename ChanT>
auto load_image_from_file_impl(
    const File& file,
    size_t      min_channels,
    size_t      max_channels,
    bool        vflip)
        -> UntypedImageLoadResult<ChanT>
{
    stbi_set_flip_vertically_on_load(vflip);
    int width, height, num_channels_in_file;

    bool ok = stbi_info(file.path().c_str(), &width, &height, &num_channels_in_file);

    if (!ok) {
        throw error::ImageReadingError(file.path(), stbi_failure_reason());
    }

    int desired_channels = std::clamp(num_channels_in_file, int(min_channels), int(max_channels));


    ChanT* data;
    if constexpr (std::same_as<ChanT, chan::UByte>) {
        data = stbi_load(
            file.path().c_str(),
            &width, &height, &num_channels_in_file,
            desired_channels
        );
    } else if constexpr (std::same_as<ChanT, chan::Float>) {
        data = stbi_loadf(
            file.path().c_str(),
            &width, &height, &num_channels_in_file,
            desired_channels
        );
    } else {
        static_assert(sizeof(ChanT) == 0);
    }

    if (!data) {
        throw error::ImageReadingError(file.path(), stbi_failure_reason());
    }

    int num_channels = desired_channels == 0 ? num_channels_in_file : desired_channels;

    return {
        unique_malloc_ptr<ChanT[]>{ data },
        Size2S(width, height),
        size_t(num_channels),
        size_t(num_channels_in_file)
    };
}


template
auto load_image_from_file_impl<chan::UByte>(
    const File& file,
    size_t      min_channels,
    size_t      max_channels,
    bool        vflip)
        -> UntypedImageLoadResult<chan::UByte>;

template
auto load_image_from_file_impl<chan::Float>(
    const File& file,
    size_t      min_channels,
    size_t      max_channels,
    bool        vflip)
        -> UntypedImageLoadResult<chan::Float>;


} // namespace detail


auto parse_cubemap_json_for_files(const File& json_file)
    -> std::array<File, 6>
{

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


} // namespace josh
