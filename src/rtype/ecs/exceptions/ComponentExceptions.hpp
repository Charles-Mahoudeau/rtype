/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when a C++ component cannot be registered: its name is already registered with another
/// layout or other options, or its description is invalid.
class ComponentRegistrationException : public std::runtime_error {
  public:
    explicit ComponentRegistrationException(const std::string& message) : std::runtime_error{message} {}
    ~ComponentRegistrationException() override = default;
    ComponentRegistrationException(const ComponentRegistrationException& other) = default;
    ComponentRegistrationException& operator=(const ComponentRegistrationException& other) = default;
    ComponentRegistrationException(ComponentRegistrationException&& other) noexcept = default;
    ComponentRegistrationException& operator=(ComponentRegistrationException&& other) noexcept = default;
};
}  // namespace rtype::ecs::exceptions
