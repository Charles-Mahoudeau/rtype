/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Engine
*/

#pragma once

#include <miniaudio.h>

#include "audio/miniaudio/MiniaudioConfig.hpp"

namespace rtype::audio::miniaudio::core {
/// @brief Owns a ma_engine: the audio device and the mixer, started on construction and stopped on destruction.
class Engine {
  public:
    /// @param config The sample rate, channels and period size to mix with; 0 lets miniaudio choose.
    /// @throws exceptions::MiniaudioException If the device cannot open.
    explicit Engine(const MiniaudioConfig& config);
    ~Engine();
    Engine(const Engine& other) = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine(Engine&& other) = delete;
    Engine& operator=(Engine&& other) = delete;

    /// @brief Stops the device: nothing plays until start(); sounds keep their position.
    void stop();
    /// @brief Restarts the device after stop().
    void start();

    /// @return The engine, for the groups and sounds attached to it.
    [[nodiscard]] ma_engine* get() noexcept;

    /// @brief Sets the master volume of the engine, affecting all groups and sounds attached to it.
    /// @param volume The volume, 0.0 (silence) to 1.0 (full volume); values above 1.0 are allowed, but may distort.
    void setVolume(float volume);

  private:
    ma_engine _engine{};  ///< Never moves: every group and sound points to it.
};
}  // namespace rtype::audio::miniaudio::core
