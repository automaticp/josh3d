#include "Globals.hpp"
#include "DataPool.hpp"
#include "FrameTimer.hpp"
#include "GLObjectPool.hpp"
#include "GLObjects.hpp"
#include "TextureData.hpp"
#include "Shared.hpp"
#include "WindowSizeCache.hpp"
#include <iostream>
#include <array>
#include <memory>



namespace learn::globals {

DataPool<TextureData> texture_data_pool;
GLObjectPool<TextureHandle> texture_handle_pool{ texture_data_pool };

std::ostream& logstream{ std::clog };

Shared<TextureHandle> default_diffuse_texture;
Shared<TextureHandle> default_specular_texture;




static TextureData fill_default(std::array<unsigned char, 4> rgba) {
    TextureData img{ 2, 2, 4 };
    const size_t n_channels = img.n_channels();
    for (size_t i{ 0 }; i < img.n_pixels(); ++i) {
        const size_t idx = i * n_channels;
        img[idx + 0] = rgba[0];
        img[idx + 1] = rgba[1];
        img[idx + 2] = rgba[2];
        img[idx + 3] = rgba[3];
    }
    return img;
}


static Shared<TextureHandle> init_default_diffuse_texture() {
    auto tex = fill_default({ 0xB0, 0xB0, 0xB0, 0xFF });
    auto handle = std::make_shared<TextureHandle>();
    handle->bind().attach_data(tex).unbind();
    return handle;
}

static Shared<TextureHandle> init_default_specular_texture() {
    auto tex = fill_default({ 0x00, 0x00, 0x00, 0xFF });
    auto handle = std::make_shared<TextureHandle>();
    handle->bind().attach_data(tex).unbind();
    return handle;
}



void init_all() {
    default_diffuse_texture = init_default_diffuse_texture();
    default_specular_texture = init_default_specular_texture();
}

void clear_all() {
    texture_data_pool.clear();
    texture_handle_pool.clear();
    default_diffuse_texture = nullptr;
    default_specular_texture = nullptr;
}


} // namespace learn::globals
