/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryTerms
*/

#pragma once

namespace rtype::ecs {
/// @brief Query term: the component may be absent. `Optional<T>` yields a Mut<T> that is empty when the entity
/// lacks it, `Optional<const T>` a `const T*` that is null then. Never drives the iteration.
template <typename T>
struct Optional {};

/// @brief Query filter: the entity must have the component; its value is not accessed (works for tags).
template <typename T>
struct With {};

/// @brief Query filter: the entity must not have the component.
template <typename T>
struct Without {};

/// @brief Query filter: the entity's component was added since the query last ran.
template <typename T>
struct Added {};

/// @brief Query filter: the entity's component was added or written since the query last ran.
template <typename T>
struct Changed {};
}  // namespace rtype::ecs
