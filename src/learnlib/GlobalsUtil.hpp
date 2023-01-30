#pragma once
#include "Globals.hpp"
#include "FrameTimer.hpp"
#include "WindowSizeCache.hpp"
#include "Basis.hpp"
#include <iostream>


namespace learn::globals {


inline FrameTimer frame_timer;

// Only really usable for apps with one window.
inline WindowSizeCache window_size;

extern std::ostream& logstream;

inline const OrthonormalBasis3D basis{ glm::vec3(1.0f, 0.0, 0.0), glm::vec3(0.0f, 1.0f, 0.0f), true };


} // namespace learn::globals
