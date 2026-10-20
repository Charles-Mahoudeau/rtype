/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputMessage
*/

#include "rtype/replication/input/InputMessage.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <span>

#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/replication/MessageType.hpp"
#include "rtype/replication/input/InputLayout.hpp"

namespace rtype::replication::input {
namespace {
/// @brief The wire value of an axis of 1.
constexpr float kAxisScale = 32767.0F;

/// @brief Type (1), snapshot tick (4) and count (1).
constexpr std::size_t kHeaderSize = 6;

/// @brief Sequence (4) and buttons (4).
constexpr std::size_t kSampleFixedSize = 8;

[[nodiscard]] std::int16_t quantize(const float value) noexcept {
    if (std::isnan(value)) {
        return 0;
    }
    return static_cast<std::int16_t>(std::lround(std::clamp(value, -1.0F, 1.0F) * kAxisScale));
}

[[nodiscard]] float dequantize(const std::int16_t value) noexcept {
    return std::max(-1.0F, static_cast<float>(value) / kAxisScale);
}
}  // namespace

network::Bytes encodeInput(const InputMessage& message, const InputLayout& layout) {
    const std::size_t slots = layout.axisSlotCount();
    network::Bytes bytes(kHeaderSize + (message.inputs.size() * (kSampleFixedSize + (slots * 2))));
    network::bytes::ByteWriter writer{bytes};
    writer.writeU8(static_cast<std::uint8_t>(MessageType::kInput));
    writer.writeU32(message.lastAppliedSnapshot);
    writer.writeU8(static_cast<std::uint8_t>(message.inputs.size()));
    for (const InputSample& sample : message.inputs) {
        writer.writeU32(sample.sequence);
        writer.writeU32(sample.buttons);
        for (std::size_t slot = 0; slot < slots; ++slot) {
            // NOLINTNEXTLINE(*-constant-array-index,*-avoid-unchecked-container-access): slot < kMaxInputAxes
            writer.writeI16(quantize(sample.axes[slot]));
        }
    }
    return bytes;
}

std::optional<InputMessage> decodeInput(const std::span<const std::byte> message, const InputLayout& layout) noexcept {
    try {
        network::bytes::ByteReader reader{message};
        if (reader.readU8() != static_cast<std::uint8_t>(MessageType::kInput)) {
            return std::nullopt;
        }
        InputMessage result;
        result.lastAppliedSnapshot = reader.readU32();
        const std::size_t count = reader.readU8();
        if (!reader.isValid() || count == 0 || count > kMaxInputsPerMessage) {
            return std::nullopt;
        }
        const std::size_t buttons = layout.buttonCount();
        const std::size_t slots = layout.axisSlotCount();
        const std::uint32_t allowedButtons = buttons >= 32 ? 0xFFFFFFFFU : ((1U << buttons) - 1U);
        result.inputs.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            InputSample sample;
            sample.sequence = reader.readU32();
            sample.buttons = reader.readU32();
            for (std::size_t slot = 0; slot < slots; ++slot) {
                // NOLINTNEXTLINE(*-constant-array-index,*-avoid-unchecked-container-access): slot < kMaxInputAxes
                sample.axes[slot] = dequantize(reader.readI16());
            }
            if (!reader.isValid() || (sample.buttons & ~allowedButtons) != 0 ||
                (!result.inputs.empty() && sample.sequence <= result.inputs.back().sequence)) {
                return std::nullopt;
            }
            result.inputs.push_back(sample);
        }
        if (!reader.isFinished()) {
            return std::nullopt;
        }
        return result;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}
}  // namespace rtype::replication::input
