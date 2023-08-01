#pragma once
#include <concepts>




namespace josh {


template<typename T, typename U>
concept same_as_remove_cvref =
    std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;


template<typename T, typename ...Args>
concept not_move_or_copy_constructor_of =
    !(sizeof...(Args) == 1 && (same_as_remove_cvref<T, Args> && ...));


} // namespace josh
