/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

namespace rtype::plugins::sprite::exceptions {
/// @brief Exception thrown when a sprite atlas cannot be built: its file cannot be read or parsed, an image to pack
/// cannot be decoded or does not fit, or an animation names a frame the atlas does not have.
class AtlasException : public std::runtime_error {
  public:
    explicit AtlasException(const std::string& message) : std::runtime_error{message} {}
    ~AtlasException() override = default;
    AtlasException(const AtlasException& other) = default;
    AtlasException& operator=(const AtlasException& other) = default;
    AtlasException(AtlasException&& other) noexcept = default;
    AtlasException& operator=(AtlasException&& other) noexcept = default;
};
}  // namespace rtype::plugins::sprite::exceptions
