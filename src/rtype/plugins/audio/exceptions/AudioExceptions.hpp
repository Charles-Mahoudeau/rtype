/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

namespace rtype::plugins::audio::exceptions {
/// @brief Exception thrown when a sound cannot be loaded: its file cannot be opened, its content cannot be decoded, or
/// its path has the same id as another one.
class AudioLoadException : public std::runtime_error {
  public:
    explicit AudioLoadException(const std::string& message) : std::runtime_error{message} {}
    ~AudioLoadException() override = default;
    AudioLoadException(const AudioLoadException& other) = default;
    AudioLoadException& operator=(const AudioLoadException& other) = default;
    AudioLoadException(AudioLoadException&& other) noexcept = default;
    AudioLoadException& operator=(AudioLoadException&& other) noexcept = default;
};

/// @brief Exception thrown when the audio backend cannot be created: the config names none or one that was not
/// registered, or the device cannot open while `audio.required` is true.
class AudioBackendException : public std::runtime_error {
  public:
    explicit AudioBackendException(const std::string& message) : std::runtime_error{message} {}
    ~AudioBackendException() override = default;
    AudioBackendException(const AudioBackendException& other) = default;
    AudioBackendException& operator=(const AudioBackendException& other) = default;
    AudioBackendException(AudioBackendException&& other) noexcept = default;
    AudioBackendException& operator=(AudioBackendException&& other) noexcept = default;
};
}  // namespace rtype::plugins::audio::exceptions
