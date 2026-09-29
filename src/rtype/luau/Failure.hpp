/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Failure
*/

#pragma once

#include <source_location>
#include <string>

#include "Error.hpp"
#include "ErrorKind.hpp"
#include "Export.hpp"
#include "Result.hpp"

namespace rtype::luau {
/// @brief Error carrier implicitly convertible to any `Result<T>`, so failures never spell the backing type.
///
/// The source location defaults to the place where the `Failure` is constructed.
class RTYPE_LUAU_API Failure {
  public:
    /// @brief Builds a failure with a traceback.
    Failure(ErrorKind kind, std::string message, std::string traceback,
            std::source_location location = std::source_location::current()) noexcept;
    /// @brief Builds a failure without a traceback.
    Failure(ErrorKind kind, std::string message,
            std::source_location location = std::source_location::current()) noexcept;
    ~Failure() noexcept = default;
    Failure(const Failure&) noexcept = delete;
    Failure& operator=(const Failure&) noexcept = delete;
    Failure(Failure&&) noexcept = default;
    Failure& operator=(Failure&&) noexcept = default;

    /// @brief Converts the failure into the error side of any `Result<T>`.
    template <typename T>
    // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
    [[nodiscard]] operator Result<T>() && noexcept;

  private:
    Error _error;  ///< The error handed over to the `Result`.
};
}  // namespace rtype::luau

#include "Failure.tpp"
