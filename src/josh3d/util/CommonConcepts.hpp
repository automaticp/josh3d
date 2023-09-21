#pragma once
#include <concepts>




namespace josh {


template<typename T, typename U>
concept same_as_remove_cvref =
    std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;


template<typename T, typename ...Others>
concept any_of = (std::same_as<T, Others> || ...);


template<typename T, typename ...Others>
concept any_of_remove_cvref = (same_as_remove_cvref<T, Others> || ...);


template<typename T, typename ...Others>
concept derived_from_any_of = (std::derived_from<T, Others> || ...);


template<typename T, typename ...Args>
concept not_move_or_copy_constructor_of =
    !(sizeof...(Args) == 1 && (same_as_remove_cvref<T, Args> && ...));


} // namespace josh
