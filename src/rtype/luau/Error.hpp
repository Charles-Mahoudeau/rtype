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
class Error : public std::exception {
  public:
    Error(ErrorKind kind, std::string message, std::string traceback, std::source_location location) noexcept;
    Error(ErrorKind kind, std::string message, std::string traceback) noexcept;
    Error(ErrorKind kind, std::string message, std::source_location location) noexcept;
    Error(ErrorKind kind, std::string message) noexcept;
    ~Error() noexcept override = default;
    Error(const Error&) noexcept = delete;
    Error& operator=(const Error&) noexcept = delete;
    Error(Error&&) noexcept = default;
    Error& operator=(Error&&) noexcept = default;

    [[nodiscard]] ErrorKind kind() const noexcept;
    [[nodiscard]] std::string_view message() const noexcept;
    [[nodiscard]] const char* what() const noexcept override;

  private:
    ErrorKind _kind;
    std::string _message;
    std::string _traceback;
    std::source_location _location;
};
}  // namespace rtype::luau
