/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PrefabException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when a prefab is misused: an unknown or duplicate name, an unknown component, a value of the
/// wrong size, or child prefabs that would contain each other. The message names the prefab involved.
/// @details Exported, with its destructor defined in the library: it is thrown inside the library and caught in the
/// game, so its type information must be one, shared object (macOS compares exception types by address).
class RTYPE_ECS_API PrefabException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the prefab involved.
    explicit PrefabException(const std::string& message);
    ~PrefabException() override;
    PrefabException(const PrefabException& other) = default;
    PrefabException& operator=(const PrefabException& other) = default;
    PrefabException(PrefabException&& other) noexcept = default;
    PrefabException& operator=(PrefabException&& other) noexcept = default;
};
}  // namespace rtype::ecs::exceptions
