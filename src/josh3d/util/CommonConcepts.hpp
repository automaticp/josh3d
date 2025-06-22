#pragma once
// IWYU pragma: always_keep
#include <concepts>
#include <type_traits>




namespace josh {


template<typename...>
constexpr bool false_v = false;

/*
Disables template argument deduction on a given argument.
*/
template<typename T>
using not_deduced = std::type_identity_t<T>;


template<typename T>
concept not_void = !std::same_as<T, void>;


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


// Basic attempt to detect where T&& *actually* binds to the nonconst rvalue reference.
// That is, we can move the argument.
template<typename T>
concept forwarded_as_rvalue = std::is_rvalue_reference_v<T&&> && !std::is_const_v<T>;


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


template<typename CallableT, typename Signature>
struct signature_test;

template<typename CallableT, typename RetT, typename ...ArgTs>
struct signature_test<CallableT, RetT(ArgTs...)>
{
    static constexpr bool is_invocable = std::is_invocable_v<CallableT, ArgTs...>;
    static constexpr bool is_same_return_type = std::is_same_v<std::invoke_result_t<CallableT, ArgTs...>, RetT>;
};

template<typename CallableT, typename Signature>
concept of_signature =
    signature_test<CallableT, Signature>::is_invocable &&
    signature_test<CallableT, Signature>::is_same_return_type;


template<typename T>
concept trivially_copyable = std::is_trivially_copyable_v<T>;


} // namespace josh
