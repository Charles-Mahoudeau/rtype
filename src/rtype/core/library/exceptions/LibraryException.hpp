/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LibraryException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace rtype::core::library::exceptions {
/// @brief Exception thrown when a library cannot be used by this engine: its message names the file.
class RTYPE_CORE_API LibraryException : public std::runtime_error {
  public:
    explicit LibraryException(const std::string& message) : std::runtime_error{message} {}
    ~LibraryException() override = default;
    LibraryException(const LibraryException& other) = default;
    LibraryException& operator=(const LibraryException& other) = default;
    LibraryException(LibraryException&& other) noexcept = default;
    LibraryException& operator=(LibraryException&& other) noexcept = default;
};
}  // namespace rtype::core::library::exceptions
