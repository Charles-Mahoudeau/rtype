/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

namespace rtype::plugins::tilemap::exceptions {
/// @brief Exception thrown when a tilemap cannot be built: its Tiled file or one of its tilesets cannot be read or
/// parsed, or uses something this plugin does not support (a non-orthogonal map, compressed tile data, an image
/// collection tileset).
class TilemapException : public std::runtime_error {
  public:
    explicit TilemapException(const std::string& message) : std::runtime_error{message} {}
    ~TilemapException() override = default;
    TilemapException(const TilemapException& other) = default;
    TilemapException& operator=(const TilemapException& other) = default;
    TilemapException(TilemapException&& other) noexcept = default;
    TilemapException& operator=(TilemapException&& other) noexcept = default;
};
}  // namespace rtype::plugins::tilemap::exceptions
