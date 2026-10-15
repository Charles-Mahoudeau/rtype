/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Field
*/

#pragma once

#include <cstdint>
#include <string>

namespace rtype::ecs {
/// @brief Types a field of a component or a resource can have, understood by reflection, scripting and
/// replication.
enum class FieldType : std::uint8_t {
    kBool,     ///< `bool`.
    kI8,       ///< `std::int8_t`.
    kI16,      ///< `std::int16_t`.
    kI32,      ///< `std::int32_t`.
    kI64,      ///< `std::int64_t`.
    kU8,       ///< `std::uint8_t`.
    kU16,      ///< `std::uint16_t`.
    kU32,      ///< `std::uint32_t`.
    kU64,      ///< `std::uint64_t`.
    kF32,      ///< `float`.
    kF64,      ///< `double`.
    kVec2,     ///< Two floats (glm layout).
    kVec3,     ///< Three floats (glm layout).
    kVec4,     ///< Four floats (glm layout).
    kEntity,   ///< An Entity: remapped by the network, cleared when its target is released.
    kAssetId,  ///< A stable 64-bit asset identifier, the same in every process.
};

/// @brief Size and alignment of one value of a FieldType, in bytes.
struct FieldLayout {
    std::uint32_t size;       ///< Bytes taken by one value.
    std::uint32_t alignment;  ///< Required alignment of one value.
};

/// @return The size and alignment of one value of the given type.
[[nodiscard]] constexpr FieldLayout getFieldLayout(FieldType type) noexcept {
    switch (type) {
        case FieldType::kBool:
        case FieldType::kI8:
        case FieldType::kU8:
            return {.size = 1, .alignment = 1};
        case FieldType::kI16:
        case FieldType::kU16:
            return {.size = 2, .alignment = 2};
        case FieldType::kI32:
        case FieldType::kU32:
        case FieldType::kF32:
            return {.size = 4, .alignment = 4};
        case FieldType::kI64:
        case FieldType::kU64:
        case FieldType::kF64:
        case FieldType::kEntity:
        case FieldType::kAssetId:
            return {.size = 8, .alignment = 8};
        case FieldType::kVec2:
            return {.size = 8, .alignment = 4};
        case FieldType::kVec3:
            return {.size = 12, .alignment = 4};
        case FieldType::kVec4:
            return {.size = 16, .alignment = 4};
    }
    return {.size = 0, .alignment = 1};  // Unreachable: every FieldType is handled above.
}

/// @brief One reflected field of a component or a resource.
struct FieldInfo {
    std::string name;                  ///< Field name, unique within its type.
    FieldType type{FieldType::kBool};  ///< Type of each value.
    std::uint32_t offset{0};           ///< Byte offset of the first value inside the type's value.
    std::uint32_t count{1};            ///< Number of values: 1 for a scalar, N for a fixed array.
};
}  // namespace rtype::ecs
