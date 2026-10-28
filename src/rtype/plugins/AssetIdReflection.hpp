/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetIdReflection
*/

#pragma once

#include <type_traits>

#include "core/asset/AssetId.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

/// @brief An AssetId field is reflected as kAssetId: written as is by the network, the same in every process.
template <>
struct rtype::ecs::FieldTypeOf<rtype::core::asset::AssetId>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kAssetId> {};
