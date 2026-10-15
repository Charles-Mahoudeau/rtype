/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceInfo
*/

#include "ResourceInfo.hpp"

#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
Result<void> validate(const ResourceInfo& info) { return validateType(info.type, "resource"); }
}  // namespace rtype::ecs
