/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputMessage
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/replication/Export.hpp"
#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace rtype::replication::input {
/// @brief The most inputs one kInput message carries.
inline constexpr std::size_t kMaxInputsPerMessage = 3;

/// @brief One input of the player: held states, never events.
struct InputSample {
    std::uint32_t sequence{0};                           ///< The client's own input counter.
    std::uint32_t buttons{0};                            ///< Bit `i` set: button action `i` is held.
    std::array<float, component::kMaxInputAxes> axes{};  ///< The axis slots, each in [-1, 1].
};

/// @brief The kInput message: the last inputs of a client.
struct InputMessage {
    std::uint32_t lastAppliedSnapshot{0};  ///< The tick of the last snapshot the client applied.
    std::vector<InputSample> inputs;       ///< 1 to 3, oldest first, sequences strictly increasing.
};

/// @brief Writes a kInput message. It is not a Codec: the number of axes comes from the layout.
/// @param message The message, with 1 to kMaxInputsPerMessage inputs in increasing sequence.
/// @param layout The layout both sides share.
/// @return The bytes, starting with the message type.
[[nodiscard]] RTYPE_REPLICATION_API network::Bytes encodeInput(const InputMessage& message, const InputLayout& layout);

/// @brief Reads a kInput message from bytes of the network; never throws.
/// @param message The whole message, starting with the message type.
/// @param layout The layout both sides share.
/// @return The message, or nothing when the count is not 1 to 3, the sequences do not strictly increase, a button
/// bit beyond the layout is set, or bytes are missing or left over.
[[nodiscard]] RTYPE_REPLICATION_API std::optional<InputMessage> decodeInput(std::span<const std::byte> message,
                                                                            const InputLayout& layout) noexcept;
}  // namespace rtype::replication::input
