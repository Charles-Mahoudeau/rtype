/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Engine
*/

#include "audio/miniaudio/core/Engine.hpp"

#include <miniaudio.h>

#include "audio/miniaudio/MiniaudioConfig.hpp"
#include "audio/miniaudio/exceptions/MiniaudioException.hpp"

namespace rtype::audio::miniaudio::core {
Engine::Engine(const MiniaudioConfig& config) {
    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.sampleRate = config.sampleRate;
    engineConfig.channels = config.channels;
    engineConfig.periodSizeInMilliseconds = config.periodSizeInMilliseconds;
    const ma_result result = ma_engine_init(&engineConfig, &_engine);
    if (result != MA_SUCCESS) {
        throw exceptions::MiniaudioException("Failed to initialize the engine", result);
    }
}

Engine::~Engine() { ma_engine_uninit(&_engine); }

ma_engine* Engine::get() noexcept { return &_engine; }

void Engine::stop() { ma_engine_stop(&_engine); }

void Engine::start() { ma_engine_start(&_engine); }

void Engine::setVolume(float volume) { ma_engine_set_volume(&_engine, volume); }

}  // namespace rtype::audio::miniaudio::core
