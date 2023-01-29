#include "Globals.hpp"
#include "TextureData.hpp"
#include "Shared.hpp"
#include "GLObjects.hpp"
#include <array>
#include <memory>


namespace learn::globals {


static TextureData fill_default(std::array<unsigned char, 4> rgba) {
    TextureData img{ ImageData(2, 2, 4) };
    unsigned char* data = img.data();
    size_t n_channels = img.n_channels();
    for (size_t i{ 0 }; i < img.width() * img.height(); ++i) {
        auto idx = i * n_channels;
        data[idx + 0] = rgba[0];
        data[idx + 1] = rgba[1];
        data[idx + 2] = rgba[2];
        data[idx + 3] = rgba[3];
    }
    return img;
}


Shared<TextureHandle> init_default_diffuse_texture() {
    auto tex = fill_default({ 0xB0, 0xB0, 0xB0, 0xFF });
    auto handle = std::make_shared<TextureHandle>();
    handle->bind().attach_data(tex).unbind();
    return handle;
}

Shared<TextureHandle> init_default_specular_texture() {
    auto tex = fill_default({ 0x00, 0x00, 0x00, 0xFF });
    auto handle = std::make_shared<TextureHandle>();
    handle->bind().attach_data(tex).unbind();
    return handle;
}



} // namespace learn::globals
