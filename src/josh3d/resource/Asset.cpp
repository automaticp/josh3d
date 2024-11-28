#include "Asset.hpp"


namespace josh {


auto image_intent_minmax_channels(ImageIntent intent) noexcept
    -> std::pair<size_t, size_t>
{
    size_t min_channels = 0;
    size_t max_channels = 4;
    switch (intent) {
        case ImageIntent::Albedo:
            min_channels = 3;
            max_channels = 4;
            break;
        case ImageIntent::Specular:
            min_channels = 1;
            max_channels = 1;
            break;
        case ImageIntent::Normal:
            min_channels = 3;
            max_channels = 3;
            break;
        case ImageIntent::Alpha: // NOLINT(bugprone-branch-clone)
            min_channels = 1;
            max_channels = 1;
            break;
        case ImageIntent::Heightmap:
            min_channels = 1;
            max_channels = 1;
            break;
        default:
            break;
    };
    return { min_channels, max_channels };
};





} // namespace josh
