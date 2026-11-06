/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleExceptions
*/

#pragma once

#include <stdexcept>
#include <string>

#include "core/Export.hpp"

namespace rtype::contract::renderer::exceptions {
/// @brief Exception thrown when a particle effect is invalid: curve keys out of order, a range whose min is above its
/// max... Exported (RTYPE_HEADER_ONLY_API) so its type is the same in every binary: the renderers, shared libraries,
/// throw it when they bake an effect, and the game catches it.
class RTYPE_HEADER_ONLY_API ParticleException : public std::runtime_error {
  public:
    explicit ParticleException(const std::string& message) : std::runtime_error(message) {}
};
}  // namespace rtype::contract::renderer::exceptions
