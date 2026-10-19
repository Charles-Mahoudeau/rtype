/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

namespace rtype::plugins::backend::exceptions {
/// @brief Exception thrown when an asset cannot be loaded: its file cannot be read, or its content cannot be decoded.
class AssetLoadException : public std::runtime_error {
  public:
    explicit AssetLoadException(const std::string& message) : std::runtime_error{message} {}
    ~AssetLoadException() override = default;
    AssetLoadException(const AssetLoadException& other) = default;
    AssetLoadException& operator=(const AssetLoadException& other) = default;
    AssetLoadException(AssetLoadException&& other) noexcept = default;
    AssetLoadException& operator=(AssetLoadException&& other) noexcept = default;
};
}  // namespace rtype::plugins::backend::exceptions
