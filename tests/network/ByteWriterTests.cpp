/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ByteWriterTests
*/

#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include "rtype/network/bytes/ByteWriter.hpp"

namespace bytes = rtype::network::bytes;

namespace {

template <typename... Ts>
constexpr std::array<std::byte, sizeof...(Ts)> makeBytes(Ts... values) {
    return {static_cast<std::byte>(values)...};
}

}  // namespace

TEST(ByteWriter, StartsValidAndEmpty) {
    std::array<std::byte, 4> buffer{};
    const bytes::ByteWriter writer{buffer};

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 0U);
    EXPECT_TRUE(writer.writtenBytes().empty());
}

TEST(ByteWriter, EmptyBufferIsValidUntilAWrite) {
    bytes::ByteWriter writer{std::span<std::byte>{}};

    EXPECT_TRUE(writer.isValid());
    writer.writeU8(1);
    EXPECT_FALSE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 0U);
}

TEST(ByteWriter, WriteU8) {
    std::array<std::byte, 1> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU8(0xAB);

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 1U);
    EXPECT_EQ(buffer, makeBytes(0xAB));
}

TEST(ByteWriter, WriteU16IsLittleEndian) {
    std::array<std::byte, 2> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU16(0x1234);

    EXPECT_EQ(buffer, makeBytes(0x34, 0x12));
}

TEST(ByteWriter, WriteU32IsLittleEndian) {
    std::array<std::byte, 4> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU32(0x12345678U);

    EXPECT_EQ(buffer, makeBytes(0x78, 0x56, 0x34, 0x12));
}

TEST(ByteWriter, WriteU64IsLittleEndian) {
    std::array<std::byte, 8> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU64(0x0123456789ABCDEFULL);

    EXPECT_EQ(buffer, makeBytes(0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01));
}

TEST(ByteWriter, WriteUnsignedLimits) {
    std::array<std::byte, 15> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU8(std::numeric_limits<std::uint8_t>::max());
    writer.writeU16(std::numeric_limits<std::uint16_t>::max());
    writer.writeU32(std::numeric_limits<std::uint32_t>::max());
    writer.writeU64(std::numeric_limits<std::uint64_t>::max());

    EXPECT_TRUE(writer.isValid());
    for (const std::byte byte : buffer) {
        EXPECT_EQ(byte, std::byte{0xFF});
    }
}

TEST(ByteWriter, SignedIntegersUseTwosComplement) {
    std::array<std::byte, 15> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeI8(-1);
    writer.writeI16(-2);
    writer.writeI32(-3);
    writer.writeI64(-4);

    EXPECT_EQ(buffer,
              makeBytes(0xFF, 0xFE, 0xFF, 0xFD, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF));
}

TEST(ByteWriter, SignedMinimums) {
    std::array<std::byte, 2> buffer16{};
    bytes::ByteWriter writer16{buffer16};
    writer16.writeI16(std::numeric_limits<std::int16_t>::min());
    EXPECT_EQ(buffer16, makeBytes(0x00, 0x80));

    std::array<std::byte, 1> buffer8{};
    bytes::ByteWriter writer8{buffer8};
    writer8.writeI8(std::numeric_limits<std::int8_t>::min());
    EXPECT_EQ(buffer8, makeBytes(0x80));
}

TEST(ByteWriter, WriteF32KeepsIeeeBits) {
    std::array<std::byte, 4> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeF32(1.0F);

    // 1.0f = 0x3F800000
    EXPECT_EQ(buffer, makeBytes(0x00, 0x00, 0x80, 0x3F));
}

TEST(ByteWriter, WriteF64KeepsIeeeBits) {
    std::array<std::byte, 8> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeF64(1.0);

    // 1.0 = 0x3FF0000000000000
    EXPECT_EQ(buffer, makeBytes(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F));
}

TEST(ByteWriter, NegativeZeroKeepsItsSignBit) {
    std::array<std::byte, 4> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeF32(-0.0F);

    EXPECT_EQ(buffer, makeBytes(0x00, 0x00, 0x00, 0x80));
}

TEST(ByteWriter, NanKeepsItsExactBits) {
    std::array<std::byte, 4> buffer{};
    bytes::ByteWriter writer{buffer};
    const float nan = std::bit_cast<float>(0x7FC12345U);

    writer.writeF32(nan);

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(buffer, makeBytes(0x45, 0x23, 0xC1, 0x7F));
}

TEST(ByteWriter, WriteBytesCopiesRawBytes) {
    std::array<std::byte, 3> buffer{};
    bytes::ByteWriter writer{buffer};
    const auto source = makeBytes(0x01, 0x02, 0x03);

    writer.writeBytes(source);

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 3U);
    EXPECT_EQ(buffer, source);
}

TEST(ByteWriter, WriteEmptyBytesIsANoOp) {
    std::array<std::byte, 2> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeBytes({});

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 0U);
}

TEST(ByteWriter, WriteStringPrefixesItsLength) {
    std::array<std::byte, 4> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeString("abc");

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 4U);
    EXPECT_EQ(buffer, makeBytes(3, 'a', 'b', 'c'));
}

TEST(ByteWriter, WriteEmptyStringWritesOnlyTheLength) {
    std::array<std::byte, 1> buffer{std::byte{0xFF}};
    bytes::ByteWriter writer{buffer};

    writer.writeString("");

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 1U);
    EXPECT_EQ(buffer, makeBytes(0));
}

TEST(ByteWriter, WriteStringOf255BytesFits) {
    std::array<std::byte, 256> buffer{};
    bytes::ByteWriter writer{buffer};
    const std::string text(255, 'x');

    writer.writeString(text);

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 256U);
    EXPECT_EQ(buffer[0], std::byte{255});
    EXPECT_EQ(buffer[255], std::byte{'x'});
}

TEST(ByteWriter, WriteStringLongerThan255InvalidatesEvenWithRoom) {
    std::array<std::byte, 512> buffer{};
    bytes::ByteWriter writer{buffer};
    const std::string text(256, 'x');

    writer.writeString(text);

    EXPECT_FALSE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 0U);
}

TEST(ByteWriter, WriteZerosOverwritesWithZeros) {
    std::array<std::byte, 4> buffer{std::byte{1}, std::byte{1}, std::byte{1}, std::byte{1}};
    bytes::ByteWriter writer{buffer};

    writer.writeZeros(3);

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 3U);
    EXPECT_EQ(buffer, makeBytes(0, 0, 0, 1));
}

TEST(ByteWriter, WriteZerosOfZeroIsANoOp) {
    std::array<std::byte, 1> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeZeros(0);

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 0U);
}

TEST(ByteWriter, WritesFillExactlyTheBuffer) {
    std::array<std::byte, 4> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU32(0xDEADBEEFU);

    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), buffer.size());
}

TEST(ByteWriter, OverflowWritesNothingAndInvalidates) {
    std::array<std::byte, 3> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU32(0xFFFFFFFFU);

    EXPECT_FALSE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 0U);
    EXPECT_EQ(buffer, makeBytes(0, 0, 0));
}

TEST(ByteWriter, OverflowKeepsPreviousBytesAndSize) {
    std::array<std::byte, 3> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU8(0x11);
    writer.writeU32(0xFFFFFFFFU);

    EXPECT_FALSE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 1U);
    EXPECT_EQ(buffer, makeBytes(0x11, 0, 0));
}

TEST(ByteWriter, InvalidWriterIgnoresLaterWritesEvenIfTheyFit) {
    std::array<std::byte, 4> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU64(1);  // does not fit
    writer.writeU8(0x42);

    EXPECT_FALSE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 0U);
    EXPECT_EQ(buffer, makeBytes(0, 0, 0, 0));
}

TEST(ByteWriter, BytesOverflowWritesNothing) {
    std::array<std::byte, 2> buffer{};
    bytes::ByteWriter writer{buffer};
    const auto source = makeBytes(1, 2, 3);

    writer.writeBytes(source);

    EXPECT_FALSE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 0U);
    EXPECT_EQ(buffer, makeBytes(0, 0));
}

TEST(ByteWriter, StringOverflowWritesNothing) {
    std::array<std::byte, 3> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeString("abc");  // needs 4 bytes

    EXPECT_FALSE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 0U);
    EXPECT_EQ(buffer, makeBytes(0, 0, 0));
}

TEST(ByteWriter, ZerosOverflowWritesNothing) {
    std::array<std::byte, 2> buffer{std::byte{1}, std::byte{1}};
    bytes::ByteWriter writer{buffer};

    writer.writeZeros(3);

    EXPECT_FALSE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 0U);
    EXPECT_EQ(buffer, makeBytes(1, 1));
}

TEST(ByteWriter, HugeCountDoesNotOverflowTheBoundsCheck) {
    std::array<std::byte, 4> buffer{};
    bytes::ByteWriter writer{buffer};
    writer.writeU8(1);

    writer.writeZeros(std::numeric_limits<std::size_t>::max());

    EXPECT_FALSE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), 1U);
}

TEST(ByteWriter, WrittenBytesViewsTheCallersBuffer) {
    std::array<std::byte, 8> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU16(0x0201);
    writer.writeU8(3);

    const auto written = writer.writtenBytes();
    EXPECT_EQ(written.size(), 3U);
    EXPECT_EQ(written.data(), buffer.data());
    EXPECT_EQ(written[0], std::byte{1});
    EXPECT_EQ(written[1], std::byte{2});
    EXPECT_EQ(written[2], std::byte{3});
}

TEST(ByteWriter, SequentialWritesAreContiguous) {
    std::array<std::byte, 7> buffer{};
    bytes::ByteWriter writer{buffer};

    writer.writeU8(0xAA);
    writer.writeU16(0xBBCC);
    writer.writeU32(0x11223344U);

    EXPECT_EQ(writer.bytesWritten(), 7U);
    EXPECT_EQ(buffer, makeBytes(0xAA, 0xCC, 0xBB, 0x44, 0x33, 0x22, 0x11));
}

TEST(ByteWriter, CopyIsIndependent) {
    std::array<std::byte, 4> buffer{};
    bytes::ByteWriter writer{buffer};
    writer.writeU8(1);

    bytes::ByteWriter copy{writer};
    copy.writeU8(2);

    EXPECT_EQ(writer.bytesWritten(), 1U);
    EXPECT_EQ(copy.bytesWritten(), 2U);
}
