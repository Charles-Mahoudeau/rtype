/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Error
*/

#pragma once

#include <string>
#include <string_view>

#include "ErrorKind.hpp"
#include "Export.hpp"

namespace rtype::scripting {
/// @brief Describes a recoverable failure of the scripting bridge: the error side of Result.
///
/// @details Returned, not thrown, for failures a correct program can hit, such as a component declared by a script
/// with a duplicate field.
class RTYPE_SCRIPTING_API Error {
  public:
    /// @brief An error of the given kind, with a message describing what went wrong.
    Error(ErrorKind kind, std::string message);
    ~Error() = default;
    Error(const Error& other) = default;
    Error& operator=(const Error& other) = default;
    Error(Error&& other) noexcept = default;
    Error& operator=(Error&& other) noexcept = default;

    /// @return The category of the failure.
    [[nodiscard]] ErrorKind kind() const noexcept;

    /// @return The human-readable description of the failure, valid while this error lives.
    [[nodiscard]] std::string_view message() const noexcept;

  private:
    ErrorKind _kind;       ///< Category of the failure.
    std::string _message;  ///< Human-readable description of the failure.
};
}  // namespace rtype::scripting
