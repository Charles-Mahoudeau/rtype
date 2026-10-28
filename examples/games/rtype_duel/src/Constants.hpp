/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Constants
*/

#pragma once

#include <array>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <string_view>

#include "plugins/backend/graphics/Color.hpp"

namespace rtype_duel {

constexpr std::uint16_t kDefaultPort = 4242;
constexpr float kViewWidth = 800.0F;
constexpr float kViewHeight = 600.0F;
constexpr float kShipX = 80.0F;        ///< Distance of each ship from its edge.
constexpr float kShipSpeed = 300.0F;   ///< World units per second.
constexpr float kLaserSpeed = 600.0F;  ///< World units per second.
constexpr glm::vec2 kShipHalfSize{24.0F, 24.0F};
constexpr glm::vec2 kLaserHalfSize{30.0F, 6.0F};
constexpr std::uint32_t kPlayerCount = 2;
constexpr float kMiddleX = kViewWidth / 2.0F;  ///< Each ship stays on its side of it.
constexpr std::uint32_t kMisfireOdds = 10;     ///< One shot in kMisfireOdds misfires.
constexpr float kMisfireLifetime = 0.25F;      ///< Seconds a Misfire lives on the server.
constexpr float kShotPitchMin = 0.9F;          ///< The shot sounds play at a random pitch in [min, max]...
constexpr float kShotPitchMax = 1.1F;
constexpr float kShotVolumeMin = 0.8F;  ///< ... and at a random volume in [min, max].
constexpr float kShotVolumeMax = 1.0F;
constexpr double kMusicVolume = 0.6;  ///< The music bus, under the shots.

// The collision categories: each Collider belongs to some (layer) and collides with some (mask).
constexpr std::uint32_t kObstacleLayer = 1U << 0;
constexpr std::uint32_t kLaserLayer = 1U << 1;
constexpr std::uint32_t kLeftShipLayer = 1U << 2;
constexpr std::uint32_t kRightShipLayer = 1U << 3;

constexpr rtype::plugins::backend::graphics::Color kObstacleFill{0.25F, 0.27F, 0.32F, 1.0F};
constexpr rtype::plugins::backend::graphics::Color kObstacleOutline{0.55F, 0.6F, 0.7F, 1.0F};

constexpr std::string_view kMoveAction = "move";    ///< Up and down, a vector (only Y is used).
constexpr std::string_view kShootAction = "shoot";  ///< The left mouse button.

constexpr const char* kLeftShipSheet = "assets/r-typesheet24.gif";
constexpr const char* kRightShipSheet = "assets/r-typesheet26.gif";
constexpr const char* kLaserSheet = "assets/bullet.gif";
constexpr const char* kMusicSound = "assets/maint.ogg";
constexpr const char* kLaserSound = "assets/laser.mp3";
constexpr const char* kMisfireSound = "assets/laser-no.mp3";
constexpr const char* kExplosionSheet = "assets/r-fx-explosions.gif";
constexpr float kExplosionScale = 1.5F;  ///< The 64-pixel explosion, over a 48-pixel ship.
constexpr const char* kBackgroundMap = "assets/levels/space-background.tmj";
constexpr glm::vec2 kBackgroundSize{272.0F, 160.0F};  ///< Its images, in pixels: scaled to the view's height.
constexpr std::int32_t kBackgroundDrawLayer = -100;   ///< Under every sprite.
constexpr std::array<const char*, 2> kPlanetImages{"assets/backgrounds/space/parallax-space-ring-planet.png",
                                                   "assets/backgrounds/space/parallax-space-big-planet.png"};
constexpr std::array<glm::vec2, 2> kPlanetSizes{glm::vec2{51.0F, 115.0F}, glm::vec2{88.0F, 87.0F}};  ///< In pixels.
constexpr float kFirstPlanetDelay = 8.0F;  ///< Seconds before the first planet passes.
constexpr float kPlanetDelayMin = 25.0F;   ///< Seconds between two planets, random in [min, max].
constexpr float kPlanetDelayMax = 50.0F;
constexpr float kPlanetScaleMin = 0.6F;   ///< The size of the farthest planet, from 1 for the closest...
constexpr float kPlanetSpeedMin = 22.0F;  ///< ... and its speed, in background pixels per second (far planets: 18)...
constexpr float kPlanetSpeedMax = 34.0F;  ///< ... up to the closest one's.
constexpr std::int32_t kPlanetDrawLayer = kBackgroundDrawLayer + 3;  ///< Over the background's three layers.
constexpr const char* kFont = "assets/fonts/orbitron.json";
constexpr std::int32_t kTextLayer = 10;  ///< Over every sprite.
constexpr float kNameSize = 22.0F;       ///< The players' names, at the top of their half.
constexpr float kNameTop = 12.0F;
constexpr rtype::plugins::backend::graphics::Color kNameColor{1.0F, 1.0F, 1.0F, 0.85F};
constexpr float kWinnerSize = 52.0F;  ///< The end of the game, in the middle of the arena.
constexpr rtype::plugins::backend::graphics::Color kWinColor{1.0F, 0.85F, 0.3F, 1.0F};
constexpr rtype::plugins::backend::graphics::Color kLoseColor{1.0F, 0.45F, 0.4F, 1.0F};

}  // namespace rtype_duel
