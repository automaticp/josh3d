#pragma once
#include "DataPool.hpp"
#include "FrameTimer.hpp"
#include "GLObjectPool.hpp"
#include "TextureData.hpp"
#include "GLObjects.hpp"
#include "WindowSizeCache.hpp"
#include "Shared.hpp"

namespace learn::globals {

void clear_all();

inline DataPool<TextureData> texture_data_pool;
inline GLObjectPool<TextureHandle> texture_handle_pool{ texture_data_pool };

inline FrameTimer frame_timer;

// Only really usable for apps with one window.
inline WindowSizeCache window_size;




Shared<TextureHandle> init_default_diffuse_texture();
Shared<TextureHandle> init_default_specular_texture();


inline Shared<TextureHandle> default_diffuse_texture;
inline Shared<TextureHandle> default_specular_texture;


// Initialize the global defaults.
// Must be done righ after creating the OpenGL context.
inline void init_all() {
    default_diffuse_texture = init_default_diffuse_texture();
    default_specular_texture = init_default_specular_texture();
}



// Clear out all the global pools and textures.
// Must be done before destroying the OpenGL context.
inline void clear_all() {
    texture_data_pool.clear();
    texture_handle_pool.clear();
    default_diffuse_texture = nullptr;
    default_specular_texture = nullptr;
}




} // namespace learn::globals
