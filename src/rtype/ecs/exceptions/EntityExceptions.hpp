/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EntityExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when no entity index is left: every one of the 2^32 slots is in use or retired.
class RTYPE_ECS_API EntityLimitException : public std::runtime_error {
  public:
    explicit EntityLimitException(const std::string& message) : std::runtime_error{message} {}
    ~EntityLimitException() override = default;
    EntityLimitException(const EntityLimitException& other) = default;
    EntityLimitException& operator=(const EntityLimitException& other) = default;
    EntityLimitException(EntityLimitException&& other) noexcept = default;
    EntityLimitException& operator=(EntityLimitException&& other) noexcept = default;
};
}  // namespace rtype::ecs::exceptions
