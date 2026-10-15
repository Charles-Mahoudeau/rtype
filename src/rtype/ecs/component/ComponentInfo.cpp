/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentInfo
*/

#include "ComponentInfo.hpp"

#include <format>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
Result<void> validate(const ComponentInfo& info) {
    if (Result<void> valid = validateType(info.type, "component"); !valid) {
        return valid;
    }
    if (!info.type.copyable) {
        return fail(ErrorKind::kInvalidType,
                    std::format("Component '{}': a component must be copyable", info.type.name));
    }
    return {};
}
}  // namespace rtype::ecs
