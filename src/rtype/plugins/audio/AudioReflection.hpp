/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioReflection
*/

#pragma once

#include <type_traits>

#include "rtype/contract/audiodevice/AudioTypes.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

// The ECS side of the audio contract, which knows no ECS. The ECS reflects scalars only: a Bus field is reflected as
// its underlying byte. The specialization must sit outside the contract's namespace, in one that encloses rtype::ecs.
/// @brief A Bus field is reflected as kU8.
template <>
struct rtype::ecs::FieldTypeOf<rtype::contract::audiodevice::Bus>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kU8> {};
static_assert(sizeof(rtype::contract::audiodevice::Bus) == 1, "Bus must match FieldType::kU8");
