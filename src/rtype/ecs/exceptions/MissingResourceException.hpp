/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MissingResourceException
*/

#pragma once

#include <format>
#include <stdexcept>
#include <string_view>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when a resource is required but the world holds no value for it: nothing inserted it.
class RTYPE_ECS_API MissingResourceException : public std::runtime_error {
  public:
    /// @param name The stable name of the missing resource, e.g. "rtype.Time".
    explicit MissingResourceException(std::string_view name)
        : std::runtime_error{std::format(
              "Resource '{}' is not in the world: insert it (or add the plugin that does) before using it", name)} {}
    ~MissingResourceException() override = default;
    MissingResourceException(const MissingResourceException& other) = default;
    MissingResourceException& operator=(const MissingResourceException& other) = default;
    MissingResourceException(MissingResourceException&& other) noexcept = default;
    MissingResourceException& operator=(MissingResourceException&& other) noexcept = default;
};
}  // namespace rtype::ecs::exceptions
