/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace rtype::plugins::physics::exceptions {

/// @brief Thrown when the physics is misused, for example when a PhysicsWorld is built from invalid settings.
class RTYPE_HEADER_ONLY_API PhysicsException : public std::runtime_error {
  public:
    explicit PhysicsException(const std::string& message) : std::runtime_error{message} {}
};

}  // namespace rtype::plugins::physics::exceptions
