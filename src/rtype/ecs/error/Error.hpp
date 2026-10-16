/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Error
*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs {
/// @brief Categories of failure the ECS reports through Result.
enum class ErrorKind : std::uint8_t {
    kInvalidType,      ///< A type description (component, resource) is inconsistent: layout, fields, replication.
    kLayoutMismatch,   ///< A type name is already registered with another layout.
    kOptionsMismatch,  ///< A type name is already registered with other options (storage, presence, replication...).
    kNameCollision,    ///< Two different type names have the same hash, or a name is used by two kinds.
};

/// @brief Describes a recoverable failure: the error side of Result.
///
/// @details Returned, not thrown, by operations that may fail for a reason the caller must be able to report, such
/// as a component description coming from a script. Hot paths never return an Error: they report expected failures
/// with `bool`, `std::optional` or `nullptr`. Build one with fail() to return it from a function returning Result.
class RTYPE_ECS_API Error {
  public:
    /// @brief An error of the given kind, with a message describing what went wrong.
    Error(ErrorKind kind, std::string message);
    ~Error() = default;
    Error(const Error& other) = default;
    Error& operator=(const Error& other) = default;
    Error(Error&& other) noexcept = default;
    Error& operator=(Error&& other) noexcept = default;

    /// @return The category of the failure.
    [[nodiscard]] ErrorKind getKind() const noexcept;

    /// @return The human-readable description of the failure, valid while this error lives.
    [[nodiscard]] std::string_view getMessage() const noexcept;

  private:
    ErrorKind _kind;       ///< Category of the failure.
    std::string _message;  ///< Human-readable description of the failure.
};
}  // namespace rtype::ecs
