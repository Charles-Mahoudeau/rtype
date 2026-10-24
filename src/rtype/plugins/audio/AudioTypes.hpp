/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AudioTypes
*/

#pragma once

#include <cstdint>
#include <type_traits>

#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

namespace rtype::plugins::audio {
/// @brief A sound the backend has loaded, as TextureId is a texture the renderer has created.
struct SoundId {
    std::uint32_t value{0};  ///< 0 means "no sound".

    [[nodiscard]] bool operator==(const SoundId& other) const = default;
};

/// @brief One playback of a sound. Stale once the voice ends: every call on a stale id does nothing.
struct VoiceId {
    std::uint32_t index{0};       ///< Slot in the backend's voice table.
    std::uint32_t generation{0};  ///< Bumped when the slot is reused, so an old id never controls a new voice; 0 is
                                  ///< never handed out, so a default VoiceId is always stale.

    [[nodiscard]] bool operator==(const VoiceId& other) const = default;
};

/// @brief The buses voices are mixed into, each with its own volume, all under the master volume.
enum class Bus : std::uint8_t {
    kMusic,    ///< The music track.
    kEffects,  ///< Everything else.
};

}  // namespace rtype::plugins::audio

// The ECS reflects scalars only: a Bus field is reflected as its underlying byte. The specialization must sit outside
// the audio namespace, in one that encloses rtype::ecs.
/// @brief A Bus field is reflected as kU8.
template <>
struct rtype::ecs::FieldTypeOf<rtype::plugins::audio::Bus>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kU8> {};
static_assert(sizeof(rtype::plugins::audio::Bus) == 1, "Bus must match FieldType::kU8");

namespace rtype::plugins::audio {
/// @brief How a sound is loaded.
struct SoundDesc {
    bool stream{false};  ///< Decoded while it plays (music), instead of decoded once into memory (effects).
};

/// @brief How a voice starts. Every value but the bus and the loop can be changed while it plays.
struct PlayParams {
    Bus bus{Bus::kEffects};
    float volume{1.0F};  ///< Linear gain, 1 = unchanged.
    float pitch{1.0F};   ///< Playback speed, 1 = unchanged.
    float pan{0.0F};     ///< -1 = left, 0 = centre, 1 = right.
    bool loop{false};
    float fadeIn{0.0F};  ///< Seconds from silence to `volume`; 0 starts at once.
};
}  // namespace rtype::plugins::audio
