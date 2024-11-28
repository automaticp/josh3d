#pragma once


/*
Lightweight header to replicate std::move, std::forward, etc. in a simpler way.

Should be faster to compile too.
*/
namespace josh {
namespace detail {
template<typename T> struct remove_reference      { using type = T; };
template<typename T> struct remove_reference<T&>  { using type = T; };
template<typename T> struct remove_reference<T&&> { using type = T; };
template<typename T> using remove_reference_t = remove_reference<T>::type;
} // namespace detail
} // namespace josh


#define MOVE(...)   static_cast<::josh::detail::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)
#define FORWARD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)
