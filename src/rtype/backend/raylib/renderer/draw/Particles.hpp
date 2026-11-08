/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Particles
*/

#pragma once

#include <raylib.h>

#include <span>

#include "rtype/contract/renderer/particle/Particle.hpp"

namespace rtype::backend::raylib::renderer::draw {

/// @brief Draws the @p instances a CPU simulation handed over, one textured quad each: the flipbook frame of its life,
/// its colour, its size, and its rotation, or along its velocity and stretched by its speed.
void drawParticles(const ::Texture2D& texture, std::span<const contract::renderer::ParticleInstance> instances,
                   const contract::renderer::ParticleDraw& particles);

}  // namespace rtype::backend::raylib::renderer::draw
