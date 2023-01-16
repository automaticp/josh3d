#pragma once
#include "DataPool.hpp"
#include "FrameTimer.hpp"
#include "GLObjectPool.hpp"
#include "TextureData.hpp"
#include "GLObjects.hpp"
#include "WindowSizeCache.hpp"

namespace learn::globals {

void clear_all();

inline DataPool<TextureData> texture_data_pool;
inline GLObjectPool<TextureHandle> texture_handle_pool{ texture_data_pool };

inline FrameTimer frame_timer;

// Only really usable for apps with one window.
inline WindowSizeCache window_size;

// Clear out all the global pools.
// Must be done before destroying the OpenGL context.
inline void clear_all() {
    texture_data_pool.clear();
    texture_handle_pool.clear();
}

} // namespace learn::globals
