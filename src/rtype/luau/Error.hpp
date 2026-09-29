/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Error
*/

#pragma once

#include <exception>
#include <source_location>
#include <string>
#include <string_view>

#include "ErrorKind.hpp"

namespace rtype::luau {
/// @brief Describes a failure raised while creating or running Luau code.
///
/// Carries the failure category, a message, an optional Luau traceback and the C++ source location where the error
/// was raised. It is the error side of `Result<T>`, and it derives from `std::exception` so it can also be thrown.
/// Copying is disabled; errors are moved.
class Error : public std::exception {
  public:
    /// @brief Builds an error with a traceback and an explicit source location.
    /// @param kind Category of the failure.
    /// @param message Human-readable description of the failure.
    /// @param traceback Luau traceback captured when the failure happened.
    /// @param location C++ location the error is attributed to.
    Error(ErrorKind kind, std::string message, std::string traceback, std::source_location location) noexcept;
    /// @brief Builds an error with a traceback.
    /// @note The source location is captured inside this constructor, not at the call site. Use the overload taking a
    /// `std::source_location` (or `Failure`) to attribute the error to the caller.
    Error(ErrorKind kind, std::string message, std::string traceback) noexcept;
    /// @brief Builds an error with an explicit source location and an empty traceback.
    Error(ErrorKind kind, std::string message, std::source_location location) noexcept;
    /// @brief Builds an error with an empty traceback.
    /// @note The source location is captured inside this constructor, not at the call site.
    Error(ErrorKind kind, std::string message) noexcept;
    ~Error() noexcept override = default;
    Error(const Error&) noexcept = delete;
    Error& operator=(const Error&) noexcept = delete;
    Error(Error&&) noexcept = default;
    Error& operator=(Error&&) noexcept = default;

    /// @brief Returns the category of the failure.
    [[nodiscard]] ErrorKind kind() const noexcept;
    /// @brief Returns the human-readable description of the failure.
    /// @return A view valid as long as this error is alive and not moved from.
    [[nodiscard]] std::string_view message() const noexcept;
    /// @brief Returns the message as a null-terminated string, as required by `std::exception`.
    [[nodiscard]] const char* what() const noexcept override;

  private:
    ErrorKind _kind;                 ///< Category of the failure.
    std::string _message;            ///< Human-readable description of the failure.
    std::string _traceback;          ///< Luau traceback, empty when none was captured.
    std::source_location _location;  ///< C++ location the error is attributed to.
};
}  // namespace rtype::luau
