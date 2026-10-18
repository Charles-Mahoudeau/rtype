/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ByteReaderTests
*/

#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"

namespace bytes = rtype::network::bytes;

namespace {

template <typename... Ts>
constexpr std::array<std::byte, sizeof...(Ts)> makeBytes(Ts... values) {
    return {static_cast<std::byte>(values)...};
}

}  // namespace

TEST(ByteReader, StartsValid) {
    const auto data = makeBytes(1, 2, 3);
    const bytes::ByteReader reader{data};

    EXPECT_TRUE(reader.isValid());
    EXPECT_EQ(reader.remainingSize(), 3U);
    EXPECT_FALSE(reader.isFinished());
}

TEST(ByteReader, EmptyBufferIsValidAndFinished) {
    const bytes::ByteReader reader{std::span<const std::byte>{}};

    EXPECT_TRUE(reader.isValid());
    EXPECT_EQ(reader.remainingSize(), 0U);
    EXPECT_TRUE(reader.isFinished());
}

TEST(ByteReader, ReadU8) {
    const auto data = makeBytes(0xAB);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readU8(), 0xAB);
    EXPECT_TRUE(reader.isValid());
    EXPECT_TRUE(reader.isFinished());
}

TEST(ByteReader, ReadU16IsLittleEndian) {
    const auto data = makeBytes(0x34, 0x12);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readU16(), 0x1234U);
}

TEST(ByteReader, ReadU32IsLittleEndian) {
    const auto data = makeBytes(0x78, 0x56, 0x34, 0x12);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readU32(), 0x12345678U);
}

TEST(ByteReader, ReadU64IsLittleEndian) {
    const auto data = makeBytes(0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readU64(), 0x0123456789ABCDEFULL);
}

TEST(ByteReader, SignedIntegersUseTwosComplement) {
    const auto data =
        makeBytes(0xFF, 0xFE, 0xFF, 0xFD, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readI8(), -1);
    EXPECT_EQ(reader.readI16(), -2);
    EXPECT_EQ(reader.readI32(), -3);
    EXPECT_EQ(reader.readI64(), -4);
    EXPECT_TRUE(reader.isFinished());
}

TEST(ByteReader, SignedMinimums) {
    const auto data = makeBytes(0x80, 0x00, 0x80);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readI8(), std::numeric_limits<std::int8_t>::min());
    EXPECT_EQ(reader.readI16(), std::numeric_limits<std::int16_t>::min());
}

TEST(ByteReader, ReadF32) {
    const auto data = makeBytes(0x00, 0x00, 0x80, 0x3F);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readF32(), 1.0F);
}

TEST(ByteReader, ReadF64) {
    const auto data = makeBytes(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readF64(), 1.0);
}

TEST(ByteReader, NegativeZeroKeepsItsSignBit) {
    const auto data = makeBytes(0x00, 0x00, 0x00, 0x80);
    bytes::ByteReader reader{data};

    const float value = reader.readF32();

    EXPECT_EQ(value, 0.0F);
    EXPECT_TRUE(std::signbit(value));
}

TEST(ByteReader, NanKeepsItsExactBits) {
    const auto data = makeBytes(0x45, 0x23, 0xC1, 0x7F);
    bytes::ByteReader reader{data};

    const float value = reader.readF32();

    EXPECT_TRUE(reader.isValid());
    EXPECT_TRUE(std::isnan(value));
    EXPECT_EQ(std::bit_cast<std::uint32_t>(value), 0x7FC12345U);
}

TEST(ByteReader, ReadBytesReturnsAViewIntoTheBuffer) {
    const auto data = makeBytes(1, 2, 3, 4);
    bytes::ByteReader reader{data};

    const auto view = reader.readBytes(3);

    EXPECT_TRUE(reader.isValid());
    ASSERT_EQ(view.size(), 3U);
    EXPECT_EQ(view.data(), data.data());
    EXPECT_EQ(reader.remainingSize(), 1U);
}

TEST(ByteReader, ReadZeroBytesIsAValidEmptyRead) {
    const auto data = makeBytes(1);
    bytes::ByteReader reader{data};

    const auto view = reader.readBytes(0);

    EXPECT_TRUE(reader.isValid());
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(reader.remainingSize(), 1U);
}

TEST(ByteReader, ReadStringReadsLengthThenBytes) {
    const auto data = makeBytes(3, 'a', 'b', 'c', 0x7F);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readString(), "abc");
    EXPECT_TRUE(reader.isValid());
    EXPECT_EQ(reader.remainingSize(), 1U);
}

TEST(ByteReader, ReadEmptyString) {
    const auto data = makeBytes(0);
    bytes::ByteReader reader{data};

    EXPECT_TRUE(reader.readString().empty());
    EXPECT_TRUE(reader.isFinished());
}

TEST(ByteReader, ReadStringDoesNotCheckUtf8) {
    const auto data = makeBytes(2, 0xFF, 0xFE);
    bytes::ByteReader reader{data};

    const std::string text = reader.readString();

    EXPECT_TRUE(reader.isValid());
    ASSERT_EQ(text.size(), 2U);
    EXPECT_EQ(static_cast<unsigned char>(text[0]), 0xFFU);
    EXPECT_EQ(static_cast<unsigned char>(text[1]), 0xFEU);
}

TEST(ByteReader, ReadStringWithEmbeddedNul) {
    const auto data = makeBytes(3, 'a', 0, 'b');
    bytes::ByteReader reader{data};

    const std::string text = reader.readString();

    EXPECT_EQ(text.size(), 3U);
    EXPECT_EQ(text, (std::string{"a\0b", 3}));
}

TEST(ByteReader, ReadStringWithTruncatedBodyInvalidates) {
    const auto data = makeBytes(5, 'a', 'b');
    bytes::ByteReader reader{data};

    EXPECT_TRUE(reader.readString().empty());
    EXPECT_FALSE(reader.isValid());
}

TEST(ByteReader, ReadStringWithoutLengthInvalidates) {
    bytes::ByteReader reader{std::span<const std::byte>{}};

    EXPECT_TRUE(reader.readString().empty());
    EXPECT_FALSE(reader.isValid());
}

TEST(ByteReader, SkipMovesForward) {
    const auto data = makeBytes(1, 2, 3, 4);
    bytes::ByteReader reader{data};

    reader.skip(2);

    EXPECT_TRUE(reader.isValid());
    EXPECT_EQ(reader.remainingSize(), 2U);
    EXPECT_EQ(reader.readU8(), 3);
}

TEST(ByteReader, SkipToTheExactEndIsValid) {
    const auto data = makeBytes(1, 2);
    bytes::ByteReader reader{data};

    reader.skip(2);

    EXPECT_TRUE(reader.isValid());
    EXPECT_TRUE(reader.isFinished());
}

TEST(ByteReader, SkipPastTheEndInvalidates) {
    const auto data = makeBytes(1, 2);
    bytes::ByteReader reader{data};

    reader.skip(3);

    EXPECT_FALSE(reader.isValid());
    EXPECT_FALSE(reader.isFinished());
}

TEST(ByteReader, HugeSkipDoesNotOverflowTheBoundsCheck) {
    const auto data = makeBytes(1, 2, 3, 4);
    bytes::ByteReader reader{data};
    EXPECT_EQ(reader.readU8(), 1);

    reader.skip(std::numeric_limits<std::size_t>::max());

    EXPECT_FALSE(reader.isValid());
}

TEST(ByteReader, ReadPastTheEndReturnsZeroAndInvalidates) {
    const auto data = makeBytes(1, 2, 3);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readU32(), 0U);
    EXPECT_FALSE(reader.isValid());
}

TEST(ByteReader, FailedReadConsumesNothing) {
    const auto data = makeBytes(1, 2, 3);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readU32(), 0U);

    EXPECT_EQ(reader.remainingSize(), 3U);
}

TEST(ByteReader, InvalidReaderReturnsZeroEvenIfBytesRemain) {
    const auto data = makeBytes(1, 2, 3);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readU32(), 0U);  // does not fit
    EXPECT_EQ(reader.readU8(), 0);    // would fit, but the reader is invalid for good
    EXPECT_EQ(reader.readU16(), 0U);
    EXPECT_EQ(reader.readF32(), 0.0F);
    EXPECT_EQ(reader.readF64(), 0.0);
    EXPECT_TRUE(reader.readBytes(1).empty());
    EXPECT_TRUE(reader.readString().empty());
    EXPECT_FALSE(reader.isValid());
}

TEST(ByteReader, ReadBytesPastTheEndIsEmptyAndInvalidates) {
    const auto data = makeBytes(1, 2);
    bytes::ByteReader reader{data};

    const auto view = reader.readBytes(3);

    EXPECT_TRUE(view.empty());
    EXPECT_FALSE(reader.isValid());
    EXPECT_EQ(reader.remainingSize(), 2U);
}

TEST(ByteReader, HugeReadBytesDoesNotOverflowTheBoundsCheck) {
    const auto data = makeBytes(1, 2);
    bytes::ByteReader reader{data};

    const auto view = reader.readBytes(std::numeric_limits<std::size_t>::max());

    EXPECT_TRUE(view.empty());
    EXPECT_FALSE(reader.isValid());
}

TEST(ByteReader, IsFinishedRequiresNothingRemaining) {
    const auto data = makeBytes(1, 2);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readU8(), 1);
    EXPECT_FALSE(reader.isFinished());  // trailing byte
    EXPECT_EQ(reader.readU8(), 2);
    EXPECT_TRUE(reader.isFinished());
}

TEST(ByteReader, IsFinishedIsFalseOnceInvalid) {
    bytes::ByteReader reader{std::span<const std::byte>{}};

    EXPECT_EQ(reader.readU8(), 0);

    EXPECT_EQ(reader.remainingSize(), 0U);
    EXPECT_FALSE(reader.isFinished());
}

TEST(ByteReader, SequentialReads) {
    const auto data = makeBytes(0xAA, 0xCC, 0xBB, 0x44, 0x33, 0x22, 0x11);
    bytes::ByteReader reader{data};

    EXPECT_EQ(reader.readU8(), 0xAA);
    EXPECT_EQ(reader.readU16(), 0xBBCCU);
    EXPECT_EQ(reader.readU32(), 0x11223344U);
    EXPECT_TRUE(reader.isFinished());
}

TEST(ByteReader, CopyIsIndependent) {
    const auto data = makeBytes(1, 2);
    bytes::ByteReader reader{data};
    EXPECT_EQ(reader.readU8(), 1);

    bytes::ByteReader copy{reader};
    EXPECT_EQ(copy.readU8(), 2);

    EXPECT_EQ(reader.remainingSize(), 1U);
    EXPECT_EQ(copy.remainingSize(), 0U);
}

TEST(ByteReader, RoundTripWithByteWriter) {
    std::array<std::byte, 64> buffer{};
    bytes::ByteWriter writer{buffer};
    writer.writeU8(0xFE);
    writer.writeU16(0xBEEF);
    writer.writeU32(0xDEADBEEFU);
    writer.writeU64(0x0102030405060708ULL);
    writer.writeI8(-8);
    writer.writeI16(-1600);
    writer.writeI32(-320000);
    writer.writeI64(-6400000000LL);
    writer.writeF32(3.5F);
    writer.writeF64(-2.25);
    writer.writeString("rtype");
    writer.writeZeros(2);
    ASSERT_TRUE(writer.isValid());

    bytes::ByteReader reader{writer.writtenBytes()};

    EXPECT_EQ(reader.readU8(), 0xFE);
    EXPECT_EQ(reader.readU16(), 0xBEEFU);
    EXPECT_EQ(reader.readU32(), 0xDEADBEEFU);
    EXPECT_EQ(reader.readU64(), 0x0102030405060708ULL);
    EXPECT_EQ(reader.readI8(), -8);
    EXPECT_EQ(reader.readI16(), -1600);
    EXPECT_EQ(reader.readI32(), -320000);
    EXPECT_EQ(reader.readI64(), -6400000000LL);
    EXPECT_EQ(reader.readF32(), 3.5F);
    EXPECT_EQ(reader.readF64(), -2.25);
    EXPECT_EQ(reader.readString(), "rtype");
    EXPECT_EQ(reader.readU16(), 0U);
    EXPECT_TRUE(reader.isFinished());
}
