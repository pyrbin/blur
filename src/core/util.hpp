#pragma once

#include <type_traits>

namespace blur {
/*
    has member function
*/
template <typename T>
struct is_member_function : std::false_type {};
template <typename C, typename R, typename... Ts>
struct is_member_function<R (C::*)(Ts...) const> : std::true_type {};
template <typename T>
constexpr bool is_member_function_v = is_member_function<T>::value;

/*
    has process function
*/
template <typename T, typename = void>
struct has_process_fn : std::false_type {};
template <typename T>
struct has_process_fn<
    T, std::enable_if_t<is_member_function_v<decltype(&T::process)>>>
    : std::true_type {};
/*
    no ref
*/
template <typename T>
using no_ref_t = typename std::remove_reference<T>::type;
}  // namespace blur