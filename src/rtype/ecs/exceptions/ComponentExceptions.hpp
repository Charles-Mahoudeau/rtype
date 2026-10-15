/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentExceptions
*/

#pragma once

#include "rtype/ecs/exceptions/TypeExceptions.hpp"

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when a C++ component cannot be registered: its name is already registered with another
/// layout or other options, or its description is invalid. The component name of TypeRegistrationException, so
/// callers can catch either.
using ComponentRegistrationException = TypeRegistrationException;
}  // namespace rtype::ecs::exceptions
