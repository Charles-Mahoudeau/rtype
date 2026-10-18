/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScheduleException
*/

#pragma once

#include <stdexcept>
#include <string>

namespace rtype::ecs::exceptions {
/// @brief Exception thrown when the systems of a schedule cannot be ordered: a cycle of `before` / `after`
/// constraints, a constraint naming an unknown system or one of another phase, two systems with the same name, or two
/// systems touching the same data with no order between them. The message names the systems involved.
class ScheduleException : public std::runtime_error {
  public:
    /// @param message What is wrong, naming the systems involved.
    explicit ScheduleException(const std::string& message) : std::runtime_error{message} {}
    ~ScheduleException() override = default;
    ScheduleException(const ScheduleException& other) = default;
    ScheduleException& operator=(const ScheduleException& other) = default;
    ScheduleException(ScheduleException&& other) noexcept = default;
    ScheduleException& operator=(ScheduleException&& other) noexcept = default;
};
}  // namespace rtype::ecs::exceptions
