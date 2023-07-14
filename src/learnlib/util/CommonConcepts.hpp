#pragma once
#include <concepts>




namespace josh {




template<typename T, typename U>
concept same_as_remove_cvref =
    std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;



} // namespace josh
