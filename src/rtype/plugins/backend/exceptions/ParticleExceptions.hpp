/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleExceptions
*/

#pragma once
#include <stdexcept>
#include <string>

namespace rtype::plugins::backend::exceptions {
/// @brief Exception thrown when a particle effect is invalid: curve keys out of order, a range whose min is above its
/// max...
class ParticleException : public std::runtime_error {
  public:
    explicit ParticleException(const std::string& message) : std::runtime_error(message) {}
};
}  // namespace rtype::plugins::backend::exceptions
