/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NullBackend
*/

#include "backend/null/NullBackend.hpp"

#include <chrono>
#include <cstddef>
#include <span>
#include <vector>

#include "rtype/contract/platform/WindowConfig.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"
#include "rtype/contract/renderer/particle/ParticleSystemDesc.hpp"

namespace rtype::backend::null {

void NullPlatform::init(const contract::platform::WindowConfig& config) {
    m_size = config.size;
    m_title = config.title;
    m_open = true;
    m_closedSent = false;
}

std::vector<contract::platform::Event> NullPlatform::pollEvents() {
    if (!m_open && !m_closedSent) {
        m_closedSent = true;
        return {contract::platform::event::Closed{}};
    }
    return {};
}

double NullPlatform::getTime() const {
    static const auto start = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

contract::renderer::TextureId NullRenderer::createTexture(const contract::renderer::TextureDesc& /*desc*/,
                                                          std::span<const std::byte> /*pixels*/) {
    return {.index = m_nextTexture++, .generation = 1};
}

contract::renderer::ParticleSystemId NullRenderer::createParticleSystem(
    const contract::renderer::ParticleSystemDesc& /*desc*/) {
    return {.index = m_nextParticleSystem++, .generation = 1};
}

}  // namespace rtype::backend::null
