/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** VariantHelper
*/

#pragma once

/// @brief Aggregates several callables into a single overload set.
///
/// Inherits from every callable passed as template argument and brings all their `operator()` into scope.
/// Mostly used to build a visitor for `std::visit` out of lambdas.
///
/// @code
/// std::visit(Overload{
///                [](int value) { return std::to_string(value); },
///                [](const std::string &value) { return value; },
///            },
///            variant);
/// @endcode
///
/// @tparam Ts Callable types (lambdas, functors...) to merge. Each one must have a distinct call signature.
template <class... Ts> struct Overload : Ts... {
    using Ts::operator()...;
};

/// @brief Deduction guide allowing `Overload{lambda1, lambda2, ...}` without spelling the template arguments.
template <class... Ts> Overload(Ts...) -> Overload<Ts...>;
