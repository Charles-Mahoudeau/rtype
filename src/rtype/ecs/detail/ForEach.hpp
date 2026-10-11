/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ForEach
*/

#pragma once

/// @file
/// @brief Applies a macro to every argument of a variadic macro, for RTYPE_ECS_COMPONENT's field list.
///
/// @details `RTYPE_ECS_DETAIL_FOR_EACH(macro, type, a, b, c)` expands to `macro(type, a), macro(type, b),
/// macro(type, c)`. It is the usual C++20 `__VA_OPT__` recursion: each EXPAND level rescans its argument several
/// times, which is what lets the STEP macro run again on the remaining arguments (up to 64 of them: 4 x 4 x 4 rescans).
/// MSVC needs its standard-conforming preprocessor (`/Zc:preprocessor`, set by the ecs target) for `__VA_OPT__`.

#define RTYPE_ECS_DETAIL_PARENS ()

#define RTYPE_ECS_DETAIL_EXPAND(...) \
    RTYPE_ECS_DETAIL_EXPAND_3(       \
        RTYPE_ECS_DETAIL_EXPAND_3(RTYPE_ECS_DETAIL_EXPAND_3(RTYPE_ECS_DETAIL_EXPAND_3(__VA_ARGS__))))
#define RTYPE_ECS_DETAIL_EXPAND_3(...) \
    RTYPE_ECS_DETAIL_EXPAND_2(         \
        RTYPE_ECS_DETAIL_EXPAND_2(RTYPE_ECS_DETAIL_EXPAND_2(RTYPE_ECS_DETAIL_EXPAND_2(__VA_ARGS__))))
#define RTYPE_ECS_DETAIL_EXPAND_2(...) \
    RTYPE_ECS_DETAIL_EXPAND_1(         \
        RTYPE_ECS_DETAIL_EXPAND_1(RTYPE_ECS_DETAIL_EXPAND_1(RTYPE_ECS_DETAIL_EXPAND_1(__VA_ARGS__))))
#define RTYPE_ECS_DETAIL_EXPAND_1(...) __VA_ARGS__

#define RTYPE_ECS_DETAIL_FOR_EACH(macro, type, ...) \
    __VA_OPT__(RTYPE_ECS_DETAIL_EXPAND(RTYPE_ECS_DETAIL_FOR_EACH_STEP(macro, type, __VA_ARGS__)))
#define RTYPE_ECS_DETAIL_FOR_EACH_STEP(macro, type, first, ...) \
    macro(type, first) __VA_OPT__(, RTYPE_ECS_DETAIL_FOR_EACH_AGAIN RTYPE_ECS_DETAIL_PARENS(macro, type, __VA_ARGS__))
#define RTYPE_ECS_DETAIL_FOR_EACH_AGAIN() RTYPE_ECS_DETAIL_FOR_EACH_STEP
