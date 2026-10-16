/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceInfo
*/

#pragma once

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
/// @brief Dense index of a resource type, local to one world. Use the resource's name across worlds.
using ResourceId = TypeId;

/// @brief The id of no resource.
inline constexpr ResourceId kInvalidResourceId = kInvalidTypeId;

/// @brief Everything the ECS knows about a resource type: a single value owned by the world (the time, the input,
/// the renderer), whether it comes from C++ or from a Luau script.
///
/// @details What every kind of ECS data shares (name, layout, hooks, copyability, fields, network options) is in
/// `type`; only the main-thread flag is specific to resources. Unlike a component, a resource may be any class
/// (constructors, private members, move-only): the platform and the renderer are resources. Check a description with
/// validate() before using it.
struct ResourceInfo {
    TypeInfo type;        ///< Name, layout, hooks, copyability, fields and network options.
    bool nonSend{false};  ///< Main-thread only (GLFW, Vulkan): systems using it must run on the main thread.
};

/// @brief Checks that a resource description is consistent: the rules of every type (validateType()). A resource
/// may be move-only.
/// @return Nothing on success, or an ErrorKind::kInvalidType error naming the resource and the problem.
[[nodiscard]] RTYPE_ECS_API Result<void> validate(const ResourceInfo& info);
}  // namespace rtype::ecs
