/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MiniaudioException
*/

#pragma once

#include <miniaudio.h>

#include <stdexcept>
#include <string>

namespace rtype::audio::miniaudio::exceptions {
/// @brief Exception thrown when a miniaudio call fails: the device cannot open, a sound cannot be decoded...
/// @details Its message is the given description, followed by miniaudio's description of the result and its code, e.g.
/// "Failed to initialize the engine: No backend (code -203)". The AudioRegistry turns it into a fallback to
/// the "null" backend when the device cannot open.
class MiniaudioException : public std::runtime_error {
  public:
    /// @param message Description of the failed operation.
    /// @param result What the failing miniaudio call returned.
    MiniaudioException(const std::string& message, ma_result result);
    ~MiniaudioException() override = default;
    MiniaudioException(const MiniaudioException& other) = default;
    MiniaudioException& operator=(const MiniaudioException& other) = default;
    MiniaudioException(MiniaudioException&& other) noexcept = default;
    MiniaudioException& operator=(MiniaudioException&& other) noexcept = default;

    /// @return What the failing miniaudio call returned.
    [[nodiscard]] ma_result getResult() const noexcept;

  private:
    ma_result _result;  ///< What the failing miniaudio call returned.
};
}  // namespace rtype::audio::miniaudio::exceptions
