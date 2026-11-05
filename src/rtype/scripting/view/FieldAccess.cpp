/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldAccess
*/

#include "FieldAccess.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/Error.hpp"
#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting::view {
namespace {
/// @brief Largest integer a Luau number holds exactly (2^53), the limit of the 64-bit integer fields.
constexpr double kMaxExactInteger = 9007199254740992.0;

Error fieldError(const std::string_view component, const ecs::FieldInfo& field, const std::string_view problem) {
    return Error{ErrorKind::kInvalidValue, std::format("field '{}' of '{}': {}", field.name, component, problem)};
}

/// @return Nothing when a script can read and write the field, or the error saying why not.
Result<void> checkSupported(const std::string_view component, const ecs::FieldInfo& field,
                            const std::span<const std::byte> bytes) {
    if (field.count != 1) {
        return tl::unexpected{fieldError(component, field, "array fields are not supported")};
    }
    if (field.type == ecs::FieldType::kVec2 || field.type == ecs::FieldType::kVec3 ||
        field.type == ecs::FieldType::kVec4 || field.type == ecs::FieldType::kAssetId) {
        return tl::unexpected{fieldError(component, field, "this field type is not supported yet")};
    }
    const auto size = ecs::getFieldLayout(field.type).size;
    if (size > bytes.size() || field.offset > bytes.size() - size) {
        return tl::unexpected{fieldError(component, field, "the field is out of the component's bytes")};
    }
    return {};
}

template <typename T>
T load(const std::span<const std::byte> bytes, const ecs::FieldInfo& field) {
    T value{};
    std::memcpy(&value, bytes.subspan(field.offset, sizeof(T)).data(), sizeof(T));
    return value;
}

template <typename T>
void store(const std::span<std::byte> bytes, const ecs::FieldInfo& field, const T value) {
    std::memcpy(bytes.subspan(field.offset, sizeof(T)).data(), &value, sizeof(T));
}

template <typename T>
Result<void> writeInteger(const std::string_view component, const ecs::FieldInfo& field, std::span<std::byte> bytes,
                          const double number) {
    const auto low = std::max(static_cast<double>(std::numeric_limits<T>::min()), -kMaxExactInteger);
    const auto high = std::min(static_cast<double>(std::numeric_limits<T>::max()), kMaxExactInteger);
    if (std::isnan(number) || std::trunc(number) != number) {
        return tl::unexpected{fieldError(component, field, std::format("{} is not a whole number", number))};
    }
    if (number < low || number > high) {
        return tl::unexpected{
            fieldError(component, field, std::format("{} is out of range [{}, {}]", number, low, high))};
    }
    store<T>(bytes, field, static_cast<T>(number));
    return {};
}

template <typename T>
Result<luau::Value> readInteger(const std::string_view component, const ecs::FieldInfo& field,
                                const std::span<const std::byte> bytes) {
    constexpr auto kLimit = static_cast<std::int64_t>(kMaxExactInteger);
    const auto value = load<T>(bytes, field);
    if (std::cmp_less(value, -kLimit) || std::cmp_greater(value, kLimit)) {
        const auto message = std::format("{} is out of range [{}, {}]", value, -kLimit, kLimit);
        return tl::unexpected{fieldError(component, field, message)};
    }
    return luau::Value{static_cast<double>(value)};
}
}  // namespace

Result<luau::Value> readField(const std::string_view component, const ecs::FieldInfo& field,
                              const std::span<const std::byte> bytes, const luau::Usertype<ecs::Entity>& entities) {
    if (auto supported = checkSupported(component, field, bytes); !supported) {
        return tl::unexpected{supported.error()};
    }
    switch (field.type) {
        case ecs::FieldType::kBool:
            return luau::Value{load<std::uint8_t>(bytes, field) != 0};
        case ecs::FieldType::kI8:
            return luau::Value{static_cast<double>(load<std::int8_t>(bytes, field))};
        case ecs::FieldType::kI16:
            return luau::Value{static_cast<double>(load<std::int16_t>(bytes, field))};
        case ecs::FieldType::kI32:
            return luau::Value{static_cast<double>(load<std::int32_t>(bytes, field))};
        case ecs::FieldType::kI64:
            return readInteger<std::int64_t>(component, field, bytes);
        case ecs::FieldType::kU8:
            return luau::Value{static_cast<double>(load<std::uint8_t>(bytes, field))};
        case ecs::FieldType::kU16:
            return luau::Value{static_cast<double>(load<std::uint16_t>(bytes, field))};
        case ecs::FieldType::kU32:
            return luau::Value{static_cast<double>(load<std::uint32_t>(bytes, field))};
        case ecs::FieldType::kU64:
            return readInteger<std::uint64_t>(component, field, bytes);
        case ecs::FieldType::kF32:
            return luau::Value{static_cast<double>(load<float>(bytes, field))};
        case ecs::FieldType::kF64:
            return luau::Value{load<double>(bytes, field)};
        case ecs::FieldType::kEntity: {
            auto userdata = entities.create(load<ecs::Entity>(bytes, field));
            if (!userdata) {
                return tl::unexpected{fieldError(component, field, userdata.error().message())};
            }
            return luau::Value{std::move(*userdata)};
        }
        default:
            break;
    }
    return tl::unexpected{fieldError(component, field, "this field type is not supported yet")};
}

Result<void> writeField(const std::string_view component, const ecs::FieldInfo& field, const std::span<std::byte> bytes,
                        const luau::Value& value) {
    if (auto supported = checkSupported(component, field, bytes); !supported) {
        return supported;
    }
    if (field.type == ecs::FieldType::kBool) {
        const auto* flag = value.tryAs<bool>();
        if (flag == nullptr) {
            return tl::unexpected{fieldError(component, field, "expected a boolean")};
        }
        store<std::uint8_t>(bytes, field, *flag ? 1 : 0);
        return {};
    }
    if (field.type == ecs::FieldType::kEntity) {
        const auto* userdata = value.tryAs<luau::Userdata>();
        const auto* entity = userdata != nullptr ? userdata->tryAs<ecs::Entity>() : nullptr;
        if (entity == nullptr) {
            return tl::unexpected{fieldError(component, field, "expected an Entity")};
        }
        store<ecs::Entity>(bytes, field, *entity);
        return {};
    }
    const auto* number = value.tryAs<double>();
    if (number == nullptr) {
        return tl::unexpected{fieldError(component, field, "expected a number")};
    }
    switch (field.type) {
        case ecs::FieldType::kI8:
            return writeInteger<std::int8_t>(component, field, bytes, *number);
        case ecs::FieldType::kI16:
            return writeInteger<std::int16_t>(component, field, bytes, *number);
        case ecs::FieldType::kI32:
            return writeInteger<std::int32_t>(component, field, bytes, *number);
        case ecs::FieldType::kI64:
            return writeInteger<std::int64_t>(component, field, bytes, *number);
        case ecs::FieldType::kU8:
            return writeInteger<std::uint8_t>(component, field, bytes, *number);
        case ecs::FieldType::kU16:
            return writeInteger<std::uint16_t>(component, field, bytes, *number);
        case ecs::FieldType::kU32:
            return writeInteger<std::uint32_t>(component, field, bytes, *number);
        case ecs::FieldType::kU64:
            return writeInteger<std::uint64_t>(component, field, bytes, *number);
        case ecs::FieldType::kF32:
            store<float>(bytes, field, static_cast<float>(*number));
            return {};
        case ecs::FieldType::kF64:
            store<double>(bytes, field, *number);
            return {};
        default:
            break;
    }
    return tl::unexpected{fieldError(component, field, "this field type is not supported yet")};
}

Result<void> fillFromTable(const ecs::TypeInfo& type, const luau::Table& table, const std::span<std::byte> bytes) {
    auto entries = table.entries();
    if (!entries) {
        return fail(ErrorKind::kInvalidValue,
                    std::format("component '{}': cannot read the table: {}", type.name, entries.error().message()));
    }
    for (const auto& [key, value] : *entries) {
        const auto* name = key.tryAs<std::string>();
        if (name == nullptr) {
            return fail(ErrorKind::kInvalidValue,
                        std::format("component '{}': field names must be strings", type.name));
        }
        const auto field = std::ranges::find(type.fields, *name, &ecs::FieldInfo::name);
        if (field == type.fields.end()) {
            return fail(ErrorKind::kInvalidValue, std::format("component '{}' has no field '{}'", type.name, *name));
        }
        if (auto written = writeField(type.name, *field, bytes, value); !written) {
            return written;
        }
    }
    return {};
}
}  // namespace rtype::scripting::view
