/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Utf8
*/

#include "plugins/text/detail/Utf8.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace rtype::plugins::text::detail {

namespace {
/// @brief Bits of payload in a continuation byte (10xxxxxx).
constexpr std::uint32_t kContinuationBits = 6;
constexpr std::uint32_t kContinuationMask = 0x3FU;
constexpr std::uint32_t kContinuationTag = 0x80U;
constexpr std::uint32_t kContinuationTagMask = 0xC0U;

/// @brief The largest codepoint, and the surrogates, which UTF-8 never encodes.
constexpr char32_t kMaxCodepoint = 0x10FFFF;
constexpr char32_t kFirstSurrogate = 0xD800;
constexpr char32_t kLastSurrogate = 0xDFFF;

/// @brief How a lead byte starts a sequence.
struct Lead {
    std::size_t length = 0;   ///< Bytes in the sequence, lead included; 0 if the byte cannot start one.
    std::uint32_t value = 0;  ///< The payload bits of the lead byte.
    char32_t smallest = 0;    ///< The smallest codepoint this length may encode: below it, it is overlong.
};

/// @return How @p byte starts a sequence.
Lead leadOf(std::uint32_t byte) {
    constexpr std::uint32_t kAscii = 0x80U;
    constexpr std::uint32_t kTwoMask = 0xE0U;
    constexpr std::uint32_t kTwoTag = 0xC0U;
    constexpr std::uint32_t kThreeMask = 0xF0U;
    constexpr std::uint32_t kThreeTag = 0xE0U;
    constexpr std::uint32_t kFourMask = 0xF8U;
    constexpr std::uint32_t kFourTag = 0xF0U;
    if (byte < kAscii) {
        return Lead{.length = 1, .value = byte, .smallest = 0};
    }
    if ((byte & kTwoMask) == kTwoTag) {
        return Lead{.length = 2, .value = byte & ~kTwoMask, .smallest = 0x80};
    }
    if ((byte & kThreeMask) == kThreeTag) {
        return Lead{.length = 3, .value = byte & ~kThreeMask, .smallest = 0x800};
    }
    if ((byte & kFourMask) == kFourTag) {
        return Lead{.length = 4, .value = byte & ~kFourMask, .smallest = 0x10000};
    }
    return Lead{};
}
}  // namespace

std::u32string decodeUtf8(std::string_view text) {
    std::u32string decoded;
    decoded.reserve(text.size());
    std::size_t index = 0;
    while (index < text.size()) {
        const Lead lead = leadOf(static_cast<unsigned char>(text.at(index)));
        bool valid = lead.length != 0 && index + lead.length <= text.size();
        char32_t codepoint = lead.value;
        for (std::size_t offset = 1; valid && offset < lead.length; ++offset) {
            const std::uint32_t byte = static_cast<unsigned char>(text.at(index + offset));
            valid = (byte & kContinuationTagMask) == kContinuationTag;
            codepoint = (codepoint << kContinuationBits) | (byte & kContinuationMask);
        }
        valid = valid && codepoint >= lead.smallest && codepoint <= kMaxCodepoint &&
                (codepoint < kFirstSurrogate || codepoint > kLastSurrogate);
        if (valid) {
            decoded.push_back(codepoint);
            index += lead.length;
        } else {
            decoded.push_back(kReplacementCharacter);
            ++index;
        }
    }
    return decoded;
}

}  // namespace rtype::plugins::text::detail
