/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CodecTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/network/bytes/Codec.hpp"
#include "rtype/network/bytes/Overloaded.hpp"
#include "rtype/network/bytes/SizeStream.hpp"

namespace bytes = rtype::network::bytes;

namespace {
enum class ToyKind : std::uint8_t {
    kScalars = 0x01,
    kFlags = 0x02,
    kLabel = 0x03,
    kItems = 0x04,
    kPadded = 0x05,
    kTail = 0x06,
    kMarked = 0x07,
    kBlob = 0x08,
    kLate = 0x09,
};

enum class Color : std::uint8_t { kRed = 1, kBlue = 2 };

constexpr bool isValidWireValue(const Color color) noexcept { return color == Color::kRed || color == Color::kBlue; }

struct Scalars {
    static constexpr ToyKind kKind = ToyKind::kScalars;
    std::uint8_t a{0};
    std::int16_t b{0};
    std::uint32_t c{0};
    std::int64_t d{0};
    float e{0};
    double f{0};

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.a);
        stream.value(self.b);
        stream.value(self.c);
        stream.value(self.d);
        stream.value(self.e);
        stream.value(self.f);
    }
    bool operator==(const Scalars&) const = default;
};

struct Flags {
    static constexpr ToyKind kKind = ToyKind::kFlags;
    bool enabled{false};
    Color color{Color::kRed};
    rtype::network::DisconnectReason reason{rtype::network::DisconnectReason::kGraceful};

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.enabled);
        stream.value(self.color);
        stream.value(self.reason);
    }
    bool operator==(const Flags&) const = default;
};

struct Label {
    static constexpr ToyKind kKind = ToyKind::kLabel;
    std::string text;

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.string(self.text, 5);
    }
    bool operator==(const Label&) const = default;
};

struct Item {
    std::uint8_t id{0};
    std::uint16_t amount{0};

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.id);
        stream.value(self.amount);
    }
    bool operator==(const Item&) const = default;
};

struct Items {
    static constexpr ToyKind kKind = ToyKind::kItems;
    std::vector<Item> items;
    std::vector<std::uint8_t> small;

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.template list<std::uint16_t>(self.items, 3);
        stream.template list<std::uint8_t>(self.small, 2);
    }
    bool operator==(const Items&) const = default;
};

struct Padded {
    static constexpr ToyKind kKind = ToyKind::kPadded;
    std::uint32_t value{0};

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.value);
        stream.padTo(8);
    }
    bool operator==(const Padded&) const = default;
};

struct Tail {
    static constexpr ToyKind kKind = ToyKind::kTail;
    std::uint8_t head{0};
    std::span<const std::byte> payload;

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.head);
        stream.rest(self.payload);
    }
};

struct Marked {
    static constexpr ToyKind kKind = ToyKind::kMarked;
    std::uint16_t value{0};

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.constant(std::uint16_t{0xBEEF});
        stream.value(self.value);
    }
    bool operator==(const Marked&) const = default;
};

struct Blob {
    static constexpr ToyKind kKind = ToyKind::kBlob;
    std::array<std::byte, 3> data{};

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.bytes(self.data);
    }
    bool operator==(const Blob&) const = default;
};

/// @brief Calls a field after `rest`, which is not allowed.
struct Late {
    static constexpr ToyKind kKind = ToyKind::kLate;
    std::uint8_t value{0};
    std::span<const std::byte> payload;

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.rest(self.payload);
        stream.value(self.value);
    }
};

struct TooLong {
    static constexpr ToyKind kKind = ToyKind::kPadded;
    std::uint64_t value{0};

    template <typename Self, typename Stream>
    static void serialize(Self& self, Stream& stream) {
        stream.value(self.value);
        stream.padTo(8);
    }
};

using TooLongCodec = bytes::Codec<ToyKind, std::variant<TooLong>>;

using ToyMessage = std::variant<Scalars, Flags, Label, Items, Padded, Tail, Marked, Blob, Late>;
using ToyCodec = bytes::Codec<ToyKind, ToyMessage>;

template <typename M>
rtype::network::Bytes encodeOf(const M& message) {
    return ToyCodec::encode(message);
}

template <typename M>
std::optional<M> roundTrip(const M& message) {
    const rtype::network::Bytes encoded = ToyCodec::encode(message);
    const std::optional<ToyMessage> decoded = ToyCodec::decode(encoded);
    if (!decoded || !std::holds_alternative<M>(*decoded)) {
        return std::nullopt;
    }
    return std::get<M>(*decoded);
}

std::vector<std::byte> toBytes(const std::vector<int>& values) {
    std::vector<std::byte> result;
    for (const int value : values) {
        result.push_back(static_cast<std::byte>(value));
    }
    return result;
}
}  // namespace

TEST(Codec, RoundTripsEveryScalarType) {
    const Scalars message{.a = 200, .b = -1234, .c = 0xDEADBEEF, .d = -9'000'000'000LL, .e = 1.5F, .f = -2.25};

    EXPECT_EQ(roundTrip(message), message);
}

TEST(Codec, RoundTripsBoolAndEnums) {
    const Flags message{.enabled = true, .color = Color::kBlue, .reason = rtype::network::DisconnectReason::kKicked};

    EXPECT_EQ(roundTrip(message), message);
}

TEST(Codec, RoundTripsStringsListsAndStructItems) {
    EXPECT_EQ(roundTrip(Label{.text = "hello"}), Label{.text = "hello"});
    EXPECT_EQ(roundTrip(Label{}), Label{});

    const Items items{.items = {{1, 100}, {2, 200}, {3, 300}}, .small = {7, 8}};
    EXPECT_EQ(roundTrip(items), items);
    EXPECT_EQ(roundTrip(Items{}), Items{});
}

TEST(Codec, RoundTripsBytesAndConstants) {
    const Blob blob{.data = {std::byte{1}, std::byte{2}, std::byte{3}}};
    EXPECT_EQ(roundTrip(blob), blob);
    EXPECT_EQ(roundTrip(Marked{.value = 42}), Marked{.value = 42});
}

TEST(Codec, WritesTheKindByteThenLittleEndianFields) {
    const rtype::network::Bytes encoded = encodeOf(Marked{.value = 0x0102});

    EXPECT_EQ(encoded, toBytes({0x07, 0xEF, 0xBE, 0x02, 0x01}));
}

TEST(Codec, DecodeReturnsTheRightAlternative) {
    const rtype::network::Bytes encoded = encodeOf(Padded{.value = 9});
    const std::optional<ToyMessage> decoded = ToyCodec::decode(encoded);

    ASSERT_TRUE(decoded);
    const bool visited = std::visit(
        bytes::Overloaded{[](const Padded& padded) { return padded.value == 9; }, [](const auto&) { return false; }},
        *decoded);
    EXPECT_TRUE(visited);
}

TEST(Codec, RejectsAnEmptyInput) { EXPECT_FALSE(ToyCodec::decode({})); }

TEST(Codec, RejectsAnUnknownKind) {
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x00})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x0A, 1, 2, 3})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0xFF})));
}

TEST(Codec, RejectsInputTruncatedAtEveryByte) {
    const rtype::network::Bytes full = encodeOf(Scalars{.a = 1, .b = 2, .c = 3, .d = 4, .e = 5, .f = 6});

    for (std::size_t size = 0; size < full.size(); ++size) {
        EXPECT_FALSE(ToyCodec::decode(std::span{full}.first(size))) << "size " << size;
    }
    EXPECT_TRUE(ToyCodec::decode(full));
}

TEST(Codec, RejectsOneByteLeftOver) {
    rtype::network::Bytes encoded = encodeOf(Marked{.value = 1});
    encoded.push_back(std::byte{0});

    EXPECT_FALSE(ToyCodec::decode(encoded));
}

TEST(Codec, RejectsAnEnumOutOfRangeAndZero) {
    // Flags: bool, Color, DisconnectReason.
    EXPECT_TRUE(ToyCodec::decode(toBytes({0x02, 1, 2, 7})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x02, 1, 0, 7})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x02, 1, 3, 7})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x02, 1, 2, 0})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x02, 1, 2, 8})));
}

TEST(Codec, RejectsABoolThatIsNeitherZeroNorOne) {
    EXPECT_TRUE(ToyCodec::decode(toBytes({0x02, 0, 1, 1})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x02, 2, 1, 1})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x02, 255, 1, 1})));
}

TEST(Codec, RejectsAConstantMismatch) {
    EXPECT_TRUE(ToyCodec::decode(toBytes({0x07, 0xEF, 0xBE, 0, 0})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x07, 0xEE, 0xBE, 0, 0})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x07, 0, 0, 0, 0})));
}

TEST(Codec, RejectsAListAboveItsMaximumBeforeReadingItems) {
    // Four items announced (max 3), with no item bytes at all: rejected on the count alone.
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x04, 4, 0, 0})));
    // A huge count with no data must neither allocate nor crash.
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x04, 0xFF, 0xFF})));
    // Second list above its maximum (2).
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x04, 0, 0, 3, 1, 2, 3})));
    EXPECT_TRUE(ToyCodec::decode(toBytes({0x04, 0, 0, 2, 1, 2})));
}

TEST(Codec, RejectsAListWithMissingItems) { EXPECT_FALSE(ToyCodec::decode(toBytes({0x04, 2, 0, 1, 10, 0}))); }

TEST(Codec, RejectsAStringAboveItsMaximum) {
    EXPECT_TRUE(ToyCodec::decode(toBytes({0x03, 5, 'a', 'b', 'c', 'd', 'e'})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x03, 6, 'a', 'b', 'c', 'd', 'e', 'f'})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x03, 3, 'a'})));
}

TEST(Codec, EncodeFailsOnALimitBrokenByTheMessage) {
    EXPECT_TRUE(encodeOf(Label{.text = "toolong"}).empty());
    EXPECT_TRUE(encodeOf(Items{.items = {{}, {}, {}, {}}}).empty());
    EXPECT_TRUE(encodeOf(Items{.small = {1, 2, 3}}).empty());

    std::array<std::byte, 64> buffer{};
    bytes::ByteWriter writer{buffer};
    EXPECT_FALSE(ToyCodec::encode(Label{.text = "toolong"}, writer));
}

TEST(Codec, PadToWritesZerosOnTheWritingSide) {
    const rtype::network::Bytes encoded = encodeOf(Padded{.value = 0x01020304});

    EXPECT_EQ(encoded, toBytes({0x05, 4, 3, 2, 1, 0, 0, 0}));
}

TEST(Codec, PadToRejectsAMessageTooShort) {
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x05, 4, 3, 2, 1, 0, 0})));
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x05, 4, 3, 2, 1})));
}

TEST(Codec, PadToRejectsAMessageTooLong) { EXPECT_FALSE(ToyCodec::decode(toBytes({0x05, 4, 3, 2, 1, 0, 0, 0, 0}))); }

TEST(Codec, PadToDoesNotCheckThePaddingContent) { EXPECT_TRUE(ToyCodec::decode(toBytes({0x05, 4, 3, 2, 1, 9, 9, 9}))); }

TEST(Codec, PadToFailsWhenTheMessageIsAlreadyLongerOnTheWritingSide) {
    EXPECT_TRUE(TooLongCodec::encode(TooLong{}).empty());
    std::array<std::byte, 32> buffer{};
    bytes::ByteWriter writer{buffer};
    EXPECT_FALSE(TooLongCodec::encode(TooLong{}, writer));
}

TEST(Codec, RestTakesEveryRemainingByteAsAViewIntoTheInput) {
    const std::vector<std::byte> input = toBytes({0x06, 9, 1, 2, 3});
    const std::optional<ToyMessage> decoded = ToyCodec::decode(input);

    ASSERT_TRUE(decoded);
    const Tail& tail = std::get<Tail>(*decoded);
    EXPECT_EQ(tail.head, 9);
    ASSERT_EQ(tail.payload.size(), 3U);
    EXPECT_EQ(tail.payload.data(), input.data() + 2);
}

TEST(Codec, RestCanBeEmpty) {
    const std::vector<std::byte> input = toBytes({0x06, 9});
    const std::optional<ToyMessage> decoded = ToyCodec::decode(input);

    ASSERT_TRUE(decoded);
    EXPECT_TRUE(std::get<Tail>(*decoded).payload.empty());
}

TEST(Codec, RestIsWrittenAfterTheFields) {
    const std::vector<std::byte> payload = toBytes({7, 8});
    const rtype::network::Bytes encoded = encodeOf(Tail{.head = 1, .payload = payload});

    EXPECT_EQ(encoded, toBytes({0x06, 1, 7, 8}));
}

TEST(Codec, ACallAfterRestMakesTheStreamInvalid) {
    EXPECT_FALSE(ToyCodec::decode(toBytes({0x09, 1, 2})));
    EXPECT_TRUE(encodeOf(Late{}).empty());
}

TEST(Codec, EncodeIntoABufferTooSmallReturnsFalse) {
    const Scalars message{.a = 1};
    const std::size_t size = encodeOf(message).size();

    for (std::size_t capacity = 0; capacity < size; ++capacity) {
        std::vector<std::byte> buffer(capacity);
        bytes::ByteWriter writer{buffer};
        EXPECT_FALSE(ToyCodec::encode(message, writer)) << "capacity " << capacity;
    }
    std::vector<std::byte> exact(size);
    bytes::ByteWriter writer{exact};
    EXPECT_TRUE(ToyCodec::encode(message, writer));
    EXPECT_EQ(writer.bytesWritten(), size);
}

TEST(Codec, EncodedSizeEqualsTheCountOfSizeStream) {
    const Items items{.items = {{1, 2}, {3, 4}}, .small = {5}};
    bytes::SizeStream counter{1};
    Items::serialize(items, counter);

    ASSERT_TRUE(counter.isValid());
    EXPECT_EQ(encodeOf(items).size(), counter.size());
    EXPECT_EQ(counter.size(), 1U + 2 + (2 * 3) + 1 + 1);
}

TEST(Codec, SizeStreamMatchesEveryFieldKind) {
    const Padded padded{.value = 1};
    const Label label{.text = "abc"};
    bytes::SizeStream paddedCounter{1};
    bytes::SizeStream labelCounter{1};
    Padded::serialize(padded, paddedCounter);
    Label::serialize(label, labelCounter);

    EXPECT_EQ(paddedCounter.size(), encodeOf(padded).size());
    EXPECT_EQ(labelCounter.size(), encodeOf(label).size());
}

TEST(Codec, PeekKindReadsTheFirstByteOnly) {
    EXPECT_EQ(ToyCodec::peekKind(toBytes({0x03, 0xFF})), ToyKind::kLabel);
    EXPECT_EQ(ToyCodec::peekKind(toBytes({0x09})), ToyKind::kLate);
    EXPECT_FALSE(ToyCodec::peekKind({}));
    EXPECT_FALSE(ToyCodec::peekKind(toBytes({0x00})));
    EXPECT_FALSE(ToyCodec::peekKind(toBytes({0x0A})));
}

TEST(Codec, EveryByteSequenceOfTheSmallInputsIsDecodedWithoutCrashing) {
    for (int kind = 0; kind < 256; ++kind) {
        for (int next = 0; next < 256; next += 51) {
            const std::vector<std::byte> input = toBytes({kind, next, next, next});
            static_cast<void>(ToyCodec::decode(input));
        }
    }
    SUCCEED();
}
