#pragma once
#include <concepts>
#include <type_traits>




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


// https://stackoverflow.com/questions/31762958/check-if-class-is-a-template-specialization
template<typename T, template<typename...> typename Template>
struct is_specialization : std::false_type {};

template<template<typename...> typename Template, typename ...Args>
struct is_specialization<Template<Args...>, Template> : std::true_type {};


template<typename T, template<typename...> typename Template>
concept specialization_of = is_specialization<T, Template>::value;


// Huh?
template<typename T, template<template<typename...> typename...> typename TemplateOfTemplate>
struct is_template_template_specialization : std::false_type {};

template<template<template<typename...> typename...> typename TemplateOfTemplate, template<typename...> typename ...Args>
struct is_template_template_specialization<TemplateOfTemplate<Args...>, TemplateOfTemplate> : std::true_type {};

// If you, by any chance, just so happen to be a tiny bit confused
// (not that there would be anything confusing about this, right?),
// this checks if the type T is a specialization of a template that
// takes template-template parameters as it's template arguments
// instead of normal types.
template<typename T, template<template<typename...> typename...> typename TemplateOfTemplate>
concept template_template_specialization_of = is_template_template_specialization<T, TemplateOfTemplate>::value;



} // namespace josh
