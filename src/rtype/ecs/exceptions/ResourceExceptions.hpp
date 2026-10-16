/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceExceptions
*/

#pragma once

#include "rtype/ecs/exceptions/TypeExceptions.hpp"

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when a C++ resource cannot be registered: its name is already registered with another
/// layout or other options (presence, replication, main thread), or its description is invalid. The resource name
/// of TypeRegistrationException, so callers can catch either.
using ResourceRegistrationException = TypeRegistrationException;
}  // namespace rtype::ecs::exceptions
