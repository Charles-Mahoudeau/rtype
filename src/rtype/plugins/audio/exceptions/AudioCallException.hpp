/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioCallException
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace rtype::plugins::audio::exceptions {
/// @brief Exception thrown when an audio engine call gets an argument it cannot use, such as an unknown bus.
class RTYPE_HEADER_ONLY_API AudioCallException : public std::runtime_error {
  public:
    explicit AudioCallException(const std::string& message) : std::runtime_error{message} {}
    ~AudioCallException() override = default;
    AudioCallException(const AudioCallException& other) = default;
    AudioCallException& operator=(const AudioCallException& other) = default;
    AudioCallException(AudioCallException&& other) noexcept = default;
    AudioCallException& operator=(AudioCallException&& other) noexcept = default;
};
}  // namespace rtype::plugins::audio::exceptions
