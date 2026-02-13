#pragma once
#include <concepts>
#include <limits>


namespace josh {


template<std::floating_point T = float>
constexpr auto inf = std::numeric_limits<T>::infinity();

template<typename T>
constexpr auto vmax = std::numeric_limits<T>::max();

template<typename T>
constexpr auto vmin = std::numeric_limits<T>::lowest();


} // namespace josh
