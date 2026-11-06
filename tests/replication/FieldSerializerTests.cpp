/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldSerializerTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

#include "ServerFixtures.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/String.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/bytes/ByteWriter.hpp"
#include "rtype/replication/component/Owner.hpp"
#include "rtype/replication/serialization/EntityMapper.hpp"
#include "rtype/replication/serialization/FieldSerializer.hpp"

namespace {
using rtype::ecs::Entity;
using rtype::ecs::FieldInfo;
using rtype::ecs::FieldType;
using rtype::ecs::TypeInfo;
using rtype::network::bytes::ByteReader;
using rtype::network::bytes::ByteWriter;
using rtype::replication::serialization::computeWireSize;
using rtype::replication::serialization::EntityMapper;
using rtype::replication::serialization::isSerializable;
using rtype::replication::serialization::isVariableSize;
using rtype::replication::serialization::readFields;
using rtype::replication::serialization::writeFields;

/// @brief The one entity the test mapper tracks.
constexpr Entity kTracked{5, 1};

/// @brief The NetworkId of kTracked.
constexpr std::uint32_t kTrackedId = 42;

/// @brief Maps kTracked to kTrackedId and back, and nothing else.
class TestMapper final : public EntityMapper {
  public:
    TestMapper() = default;
    ~TestMapper() override = default;
    TestMapper(const TestMapper&) = delete;
    TestMapper& operator=(const TestMapper&) = delete;
    TestMapper(TestMapper&&) = delete;
    TestMapper& operator=(TestMapper&&) = delete;

    [[nodiscard]] std::uint32_t toNetwork(Entity entity) const noexcept override {
        return entity == kTracked ? kTrackedId : 0;
    }

    [[nodiscard]] Entity toLocal(std::uint32_t networkId) const noexcept override {
        return networkId == kTrackedId ? kTracked : Entity{};
    }
};

/// @brief A value with one field of every FieldType, and a fixed array.
struct AllTypes {
    bool flag{false};
    std::int8_t i8{0};
    std::int16_t i16{0};
    std::int32_t i32{0};
    std::int64_t i64{0};
    std::uint8_t u8{0};
    std::uint16_t u16{0};
    std::uint32_t u32{0};
    std::uint64_t u64{0};
    float f32{0.0F};
    double f64{0.0};
    std::array<float, 2> vec2{};
    std::array<float, 3> vec3{};
    std::array<float, 4> vec4{};
    Entity target;
    std::uint64_t asset{0};
    std::array<std::int16_t, 3> values{};

    friend bool operator==(const AllTypes& lhs, const AllTypes& rhs) = default;
};

/// @return One field, at `offset`.
FieldInfo makeField(const char* name, FieldType type, std::size_t offset, std::uint32_t count = 1) {
    return FieldInfo{.name = name, .type = type, .offset = static_cast<std::uint32_t>(offset), .count = count};
}

/// @return The description of AllTypes.
TypeInfo makeAllTypesInfo() {
    TypeInfo info;
    info.name = "test.AllTypes";
    info.size = sizeof(AllTypes);
    info.alignment = alignof(AllTypes);
    info.fields = {
        makeField("flag", FieldType::kBool, offsetof(AllTypes, flag)),
        makeField("i8", FieldType::kI8, offsetof(AllTypes, i8)),
        makeField("i16", FieldType::kI16, offsetof(AllTypes, i16)),
        makeField("i32", FieldType::kI32, offsetof(AllTypes, i32)),
        makeField("i64", FieldType::kI64, offsetof(AllTypes, i64)),
        makeField("u8", FieldType::kU8, offsetof(AllTypes, u8)),
        makeField("u16", FieldType::kU16, offsetof(AllTypes, u16)),
        makeField("u32", FieldType::kU32, offsetof(AllTypes, u32)),
        makeField("u64", FieldType::kU64, offsetof(AllTypes, u64)),
        makeField("f32", FieldType::kF32, offsetof(AllTypes, f32)),
        makeField("f64", FieldType::kF64, offsetof(AllTypes, f64)),
        makeField("vec2", FieldType::kVec2, offsetof(AllTypes, vec2)),
        makeField("vec3", FieldType::kVec3, offsetof(AllTypes, vec3)),
        makeField("vec4", FieldType::kVec4, offsetof(AllTypes, vec4)),
        makeField("target", FieldType::kEntity, offsetof(AllTypes, target)),
        makeField("asset", FieldType::kAssetId, offsetof(AllTypes, asset)),
        makeField("values", FieldType::kI16, offsetof(AllTypes, values), 3),
    };
    return info;
}

/// @return An AllTypes with a distinct value in every field.
AllTypes makeAllTypes() {
    AllTypes value{};
    value.flag = true;
    value.i8 = -8;
    value.i16 = -1616;
    value.i32 = -323232;
    value.i64 = -6464646464;
    value.u8 = 200;
    value.u16 = 60000;
    value.u32 = 4000000000U;
    value.u64 = 18000000000000000000U;
    value.f32 = 3.5F;
    value.f64 = -2.25;
    value.vec2 = {1.0F, 2.0F};
    value.vec3 = {3.0F, 4.0F, 5.0F};
    value.vec4 = {6.0F, 7.0F, 8.0F, -0.0F};
    value.target = kTracked;
    value.asset = 0x0123456789ABCDEFU;
    value.values = {-1, 0, 1};
    return value;
}

/// @brief A small value with a padding byte: `u16` at 0, `bool` at 2, padding at 3, `f32` at 4.
struct Small {
    std::uint16_t id{0};
    bool alive{false};
    float speed{0.0F};
};

/// @return The description of Small.
TypeInfo makeSmallInfo() {
    TypeInfo info;
    info.name = "test.Small";
    info.size = sizeof(Small);
    info.alignment = alignof(Small);
    info.fields = {
        makeField("id", FieldType::kU16, offsetof(Small, id)),
        makeField("alive", FieldType::kBool, offsetof(Small, alive)),
        makeField("speed", FieldType::kF32, offsetof(Small, speed)),
    };
    return info;
}

/// @return The description of a type with a single scalar field of `type` at offset 0.
TypeInfo makeScalarInfo(FieldType type) {
    TypeInfo info;
    info.name = "test.Scalar";
    info.size = rtype::ecs::getFieldLayout(type).size;
    info.alignment = rtype::ecs::getFieldLayout(type).alignment;
    info.fields = {makeField("value", type, 0)};
    return info;
}

/// @return The bytes of `value`.
template <typename T>
std::span<const std::byte> bytesOf(const T& value) {
    return std::as_bytes(std::span{&value, 1});
}

/// @return The bytes of `value`, writable.
template <typename T>
std::span<std::byte> writableBytesOf(T& value) {
    return std::as_writable_bytes(std::span{&value, 1});
}

/// @return The bytes writeFields() writes for `value`.
template <typename T>
std::vector<std::byte> serialize(const TypeInfo& info, const T& value) {
    const TestMapper mapper;
    std::vector<std::byte> buffer(computeWireSize(info));
    ByteWriter writer{buffer};
    writeFields(writer, info, bytesOf(value), mapper);
    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), buffer.size());
    return buffer;
}

/// @return Whether readFields() accepts `bytes` as a value of `info`, read into a scratch buffer.
bool accepts(const TypeInfo& info, std::span<const std::byte> bytes) {
    const TestMapper mapper;
    std::vector<std::byte> value(info.size);
    ByteReader reader{bytes};
    return readFields(reader, info, value, mapper);
}

/// @return The bytes of one f32 written by a ByteWriter.
std::vector<std::byte> f32Bytes(float value, std::size_t count = 1) {
    std::vector<std::byte> buffer(sizeof(float) * count);
    ByteWriter writer{buffer};
    for (std::size_t i = 0; i < count; ++i) {
        writer.writeF32(value);
    }
    return buffer;
}
/// @brief A component with a text of any length.
struct Letter {
    std::string body;
    std::int32_t stamp{0};
};
RTYPE_ECS_COMPONENT(Letter, "test.serializer.Letter", body, stamp);

/// @brief A component with lists around a plain field.
struct Crate {
    std::vector<std::uint32_t> numbers;
    std::int32_t label{0};
    std::vector<Entity> owners;
};
RTYPE_ECS_COMPONENT(Crate, "test.serializer.Crate", numbers, label, owners);

/// @return The bytes of @p crate.
std::span<std::byte> bytesOf(Crate& crate) { return std::as_writable_bytes(std::span{&crate, 1}); }

/// @return @p crate as written on the wire.
std::vector<std::byte> encodeCrate(Crate& crate, const rtype::ecs::TypeInfo& info) {
    TestMapper mapper;
    std::vector<std::byte> buffer(computeWireSize(info, bytesOf(crate)));
    rtype::network::bytes::ByteWriter writer{buffer};
    writeFields(writer, info, bytesOf(crate), mapper);
    EXPECT_TRUE(writer.isValid());
    buffer.resize(writer.bytesWritten());
    return buffer;
}
}  // namespace

TEST(FieldSerializer, RoundTripsEveryFieldType) {
    const TypeInfo info = makeAllTypesInfo();
    const AllTypes source = makeAllTypes();
    const std::vector<std::byte> wire = serialize(info, source);

    const TestMapper mapper;
    AllTypes result{};
    ByteReader reader{wire};
    ASSERT_TRUE(readFields(reader, info, writableBytesOf(result), mapper));
    EXPECT_TRUE(reader.isFinished());
    EXPECT_EQ(result, source);
}

TEST(FieldSerializer, WireSizeIsTheSumOfTheFields) {
    // 1 + 1 + 2 + 4 + 8 + 1 + 2 + 4 + 8 + 4 + 8 + 8 + 12 + 16 + 4 (entity) + 8 (asset) + 3 * 2.
    EXPECT_EQ(computeWireSize(makeAllTypesInfo()), 97U);
    EXPECT_EQ(computeWireSize(makeSmallInfo()), 7U);
    EXPECT_EQ(computeWireSize(TypeInfo{}), 0U);
}

TEST(FieldSerializer, WireSizeEqualsTheBytesWritten) {
    const TypeInfo info = makeAllTypesInfo();
    const TestMapper mapper;
    std::array<std::byte, 256> buffer{};
    ByteWriter writer{buffer};

    writeFields(writer, info, bytesOf(makeAllTypes()), mapper);
    EXPECT_TRUE(writer.isValid());
    EXPECT_EQ(writer.bytesWritten(), computeWireSize(info));
}

TEST(FieldSerializer, WritesExactBytesWithoutPadding) {
    Small value{};
    value.id = 0x0102;
    value.alive = true;
    value.speed = 1.0F;

    const std::vector<std::byte> expected{std::byte{0x02}, std::byte{0x01}, std::byte{0x01}, std::byte{0x00},
                                          std::byte{0x00}, std::byte{0x80}, std::byte{0x3F}};
    EXPECT_EQ(serialize(makeSmallInfo(), value), expected);
}

TEST(FieldSerializer, WritesANonZeroBoolByteAsOne) {
    std::uint8_t raw = 7;
    const TestMapper mapper;
    std::array<std::byte, 1> buffer{};
    ByteWriter writer{buffer};

    writeFields(writer, makeScalarInfo(FieldType::kBool), bytesOf(raw), mapper);
    EXPECT_EQ(buffer[0], std::byte{1});
}

TEST(FieldSerializer, ReadZeroesPadding) {
    const TypeInfo info = makeSmallInfo();
    Small source{};
    source.id = 7;
    source.alive = true;
    source.speed = 2.0F;
    const std::vector<std::byte> wire = serialize(info, source);

    const TestMapper mapper;
    std::array<std::byte, sizeof(Small)> result{};
    result.fill(std::byte{0xFF});
    ByteReader reader{wire};
    ASSERT_TRUE(readFields(reader, info, result, mapper));
    EXPECT_EQ(result[offsetof(Small, alive) + 1], std::byte{0});
}

TEST(FieldSerializer, RejectsNanAndInfinityInScalars) {
    const std::array<float, 3> badFloats{std::numeric_limits<float>::quiet_NaN(),
                                         std::numeric_limits<float>::infinity(),
                                         -std::numeric_limits<float>::infinity()};
    for (const float bad : badFloats) {
        EXPECT_FALSE(accepts(makeScalarInfo(FieldType::kF32), f32Bytes(bad)));
    }

    const std::array<double, 3> badDoubles{std::numeric_limits<double>::quiet_NaN(),
                                           std::numeric_limits<double>::infinity(),
                                           -std::numeric_limits<double>::infinity()};
    for (const double bad : badDoubles) {
        std::array<std::byte, sizeof(double)> buffer{};
        ByteWriter writer{buffer};
        writer.writeF64(bad);
        EXPECT_FALSE(accepts(makeScalarInfo(FieldType::kF64), buffer));
    }
}

TEST(FieldSerializer, RejectsNanAndInfinityInEveryVectorComponent) {
    const std::array<float, 3> badFloats{std::numeric_limits<float>::quiet_NaN(),
                                         std::numeric_limits<float>::infinity(),
                                         -std::numeric_limits<float>::infinity()};
    for (const FieldType type : {FieldType::kVec2, FieldType::kVec3, FieldType::kVec4}) {
        const std::size_t length = rtype::ecs::getFieldLayout(type).size / sizeof(float);
        EXPECT_TRUE(accepts(makeScalarInfo(type), f32Bytes(1.0F, length)));
        for (const float bad : badFloats) {
            for (std::size_t component = 0; component < length; ++component) {
                std::vector<std::byte> buffer(length * sizeof(float));
                ByteWriter writer{buffer};
                for (std::size_t i = 0; i < length; ++i) {
                    writer.writeF32(i == component ? bad : 1.0F);
                }
                EXPECT_FALSE(accepts(makeScalarInfo(type), buffer)) << "component " << component;
            }
        }
    }
}

TEST(FieldSerializer, RejectsABoolOtherThanZeroOrOne) {
    const TypeInfo info = makeScalarInfo(FieldType::kBool);
    EXPECT_TRUE(accepts(info, std::array{std::byte{0}}));
    EXPECT_TRUE(accepts(info, std::array{std::byte{1}}));
    EXPECT_FALSE(accepts(info, std::array{std::byte{2}}));
    EXPECT_FALSE(accepts(info, std::array{std::byte{0xFF}}));
}

TEST(FieldSerializer, RejectsTruncatedInputAtEveryLength) {
    const TypeInfo info = makeAllTypesInfo();
    const std::vector<std::byte> wire = serialize(info, makeAllTypes());

    for (std::size_t length = 0; length < wire.size(); ++length) {
        EXPECT_FALSE(accepts(info, std::span{wire}.first(length))) << "length " << length;
    }
    EXPECT_TRUE(accepts(info, wire));
}

TEST(FieldSerializer, RejectsADestinationSmallerThanTheType) {
    const TypeInfo info = makeSmallInfo();
    const std::vector<std::byte> wire = serialize(info, Small{});
    const TestMapper mapper;
    std::array<std::byte, sizeof(Small) - 1> result{};
    ByteReader reader{wire};

    EXPECT_FALSE(readFields(reader, info, result, mapper));
}

TEST(FieldSerializer, WritesEntitiesAsTheirNetworkId) {
    const TypeInfo info = makeScalarInfo(FieldType::kEntity);
    const auto wireId = [&info](Entity entity) {
        const std::vector<std::byte> wire = serialize(info, entity);
        ByteReader reader{wire};
        return reader.readU32();
    };

    EXPECT_EQ(wireId(kTracked), kTrackedId);
    EXPECT_EQ(wireId(Entity{9, 3}), 0U);
    EXPECT_EQ(wireId(Entity{}), 0U);
}

TEST(FieldSerializer, ReadsNetworkIdsAsLocalEntities) {
    const TypeInfo info = makeScalarInfo(FieldType::kEntity);
    const TestMapper mapper;
    const auto localEntity = [&](std::uint32_t networkId) {
        std::array<std::byte, sizeof(std::uint32_t)> wire{};
        ByteWriter writer{wire};
        writer.writeU32(networkId);
        Entity result{3, 3};
        ByteReader reader{wire};
        EXPECT_TRUE(readFields(reader, info, writableBytesOf(result), mapper));
        return result;
    };

    EXPECT_EQ(localEntity(kTrackedId), kTracked);
    EXPECT_TRUE(localEntity(7).isNull());
    EXPECT_TRUE(localEntity(0).isNull());
}

TEST(FieldSerializer, PlainDataWithFieldsIsSerializable) {
    EXPECT_TRUE(isSerializable(makeAllTypesInfo()));
    // Owner has a default member initializer, so a construct hook: it is still plain data.
    const TypeInfo owner = rtype::ecs::TypeTraits<rtype::replication::component::Owner>::makeTypeInfo();
    EXPECT_NE(owner.hooks.construct, nullptr);
    EXPECT_TRUE(isSerializable(owner));
}

TEST(FieldSerializer, ATypeWithoutFieldsIsNotSerializable) { EXPECT_FALSE(isSerializable(TypeInfo{})); }

TEST(FieldSerializer, ATypeWithLifecycleHooksIsNotSerializable) {
    TypeInfo withDestruct = makeSmallInfo();
    withDestruct.hooks.destruct = [](void* /*target*/) noexcept {};
    EXPECT_FALSE(isSerializable(withDestruct));

    TypeInfo withMove = makeSmallInfo();
    withMove.hooks.move = [](void* /*destination*/, void* /*source*/) noexcept {};
    EXPECT_FALSE(isSerializable(withMove));

    TypeInfo withCopy = makeSmallInfo();
    withCopy.hooks.copy = [](void* /*destination*/, const void* /*source*/) {};
    EXPECT_FALSE(isSerializable(withCopy));
}

namespace {
/// @brief A value with a string, then a number, to check that what follows the string is read at its place.
struct Named {
    rtype::ecs::String name;
    std::uint8_t level{0};
};

/// @return The description of Named.
TypeInfo makeNamedInfo() {
    TypeInfo info;
    info.name = "test.Named";
    info.size = sizeof(Named);
    info.alignment = alignof(Named);
    info.fields = {
        makeField("name", FieldType::kString, offsetof(Named, name)),
        makeField("level", FieldType::kU8, offsetof(Named, level)),
    };
    return info;
}

/// @return The bytes writeFields() writes for `value`, as many as it wrote.
std::vector<std::byte> serializeNamed(const Named& value) {
    const TestMapper mapper;
    const TypeInfo info = makeNamedInfo();
    std::vector<std::byte> buffer(computeWireSize(info));
    ByteWriter writer{buffer};
    writeFields(writer, info, bytesOf(value), mapper);
    EXPECT_TRUE(writer.isValid());
    buffer.resize(writer.bytesWritten());
    return buffer;
}
}  // namespace

TEST(FieldSerializer, OnlyAStringMakesTheSizeVariable) {
    EXPECT_TRUE(isVariableSize(makeNamedInfo()));
    EXPECT_FALSE(isVariableSize(makeAllTypesInfo()));
    EXPECT_FALSE(isVariableSize(TypeInfo{}));
}

TEST(FieldSerializer, WireSizeOfAStringIsItsUpperBound) {
    // A length and 255 bytes, then the level.
    EXPECT_EQ(computeWireSize(makeNamedInfo()), 1U + 255U + 1U);
}

TEST(FieldSerializer, WritesAStringAsItsLengthAndItsBytes) {
    const Named value{.name = "Ana", .level = 9};

    const std::vector<std::byte> expected{std::byte{3}, std::byte{'A'}, std::byte{'n'}, std::byte{'a'}, std::byte{9}};
    EXPECT_EQ(serializeNamed(value), expected);
}

TEST(FieldSerializer, WritesAnEmptyStringAsItsLengthOnly) {
    const Named value{.name = "", .level = 4};

    EXPECT_EQ(serializeNamed(value), (std::vector<std::byte>{std::byte{0}, std::byte{4}}));
}

TEST(FieldSerializer, RoundTripsAString) {
    const TypeInfo info = makeNamedInfo();
    const TestMapper mapper;
    for (const std::string text : {std::string{}, std::string{"Ana"}, std::string{"héllo"}, std::string(255, 'z')}) {
        const Named source{.name = text, .level = 7};
        const std::vector<std::byte> wire = serializeNamed(source);

        Named result{};
        ByteReader reader{wire};
        ASSERT_TRUE(readFields(reader, info, writableBytesOf(result), mapper)) << text.size();
        EXPECT_TRUE(reader.isFinished());
        EXPECT_EQ(result.name, source.name);
        EXPECT_EQ(result.level, 7);
    }
}

TEST(FieldSerializer, RefusesAStringCutShort) {
    const TypeInfo info = makeNamedInfo();
    // Announces 5 bytes and holds 2.
    const std::vector<std::byte> wire{std::byte{5}, std::byte{'a'}, std::byte{'b'}};

    EXPECT_FALSE(accepts(info, wire));
    EXPECT_FALSE(accepts(info, std::vector<std::byte>{}));
}

TEST(FieldList, IsSerializableAndOfVariableSize) {
    const rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<Crate>::makeTypeInfo();
    EXPECT_TRUE(isSerializable(info));
    EXPECT_TRUE(isVariableSize(info));
}

TEST(FieldList, GivesTheSizeOfAValueOnTheWire) {
    const rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<Crate>::makeTypeInfo();
    EXPECT_EQ(computeWireSize(info), 8U);  // two counts and the label
    Crate crate{.numbers = {1, 2, 3}, .label = 7, .owners = {kTracked}};
    EXPECT_EQ(computeWireSize(info, bytesOf(crate)), 24U);
}

TEST(FieldList, ComesBackFromTheWire) {
    const rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<Crate>::makeTypeInfo();
    Crate crate{.numbers = {1, 2, 3}, .label = 7, .owners = {kTracked}};
    const std::vector<std::byte> wire = encodeCrate(crate, info);
    EXPECT_EQ(wire.size(), 24U);

    TestMapper mapper;
    Crate read;
    rtype::network::bytes::ByteReader reader{wire};
    ASSERT_TRUE(readFields(reader, info, bytesOf(read), mapper));
    EXPECT_TRUE(reader.isFinished());
    EXPECT_EQ(read.numbers, crate.numbers);
    EXPECT_EQ(read.label, 7);
    EXPECT_EQ(read.owners, crate.owners);
}

TEST(FieldList, ComesBackEmpty) {
    const rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<Crate>::makeTypeInfo();
    Crate crate;
    const std::vector<std::byte> wire = encodeCrate(crate, info);
    TestMapper mapper;
    Crate read{.numbers = {9}};
    rtype::network::bytes::ByteReader reader{wire};
    ASSERT_TRUE(readFields(reader, info, bytesOf(read), mapper));
    EXPECT_TRUE(read.numbers.empty());
}

TEST(FieldList, LeavesOutTheEntitiesTheOtherSideDoesNotKnow) {
    const rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<Crate>::makeTypeInfo();
    Crate crate{.owners = {Entity{9, 1}, kTracked, Entity{}}};
    const std::vector<std::byte> wire = encodeCrate(crate, info);
    TestMapper mapper;
    Crate read;
    rtype::network::bytes::ByteReader reader{wire};
    ASSERT_TRUE(readFields(reader, info, bytesOf(read), mapper));
    EXPECT_EQ(read.owners, std::vector<Entity>{kTracked});
}

TEST(FieldList, RefusesACountTheBytesCannotHold) {
    const rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<Crate>::makeTypeInfo();
    std::vector<std::byte> wire(4);
    rtype::network::bytes::ByteWriter writer{wire};
    writer.writeU16(0xFFFF);  // a hostile message: no value follows
    TestMapper mapper;
    Crate read;
    rtype::network::bytes::ByteReader reader{wire};
    EXPECT_FALSE(readFields(reader, info, bytesOf(read), mapper));
    EXPECT_TRUE(read.numbers.empty());
}

TEST(FieldList, RefusesAMessageCutInTheMiddleOfAList) {
    const rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<Crate>::makeTypeInfo();
    Crate crate{.numbers = {1, 2, 3}};
    std::vector<std::byte> wire = encodeCrate(crate, info);
    wire.resize(6);
    TestMapper mapper;
    Crate read;
    rtype::network::bytes::ByteReader reader{wire};
    EXPECT_FALSE(readFields(reader, info, bytesOf(read), mapper));
}

TEST(FieldText, ComesBackFromTheWireWhateverItsLength) {
    const rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<Letter>::makeTypeInfo();
    EXPECT_TRUE(isSerializable(info));
    EXPECT_TRUE(isVariableSize(info));
    Letter letter{.body = std::string(600, 'z'), .stamp = 3};
    const std::span<std::byte> bytes = std::as_writable_bytes(std::span{&letter, 1});
    TestMapper mapper;
    std::vector<std::byte> wire(computeWireSize(info, bytes));
    EXPECT_EQ(wire.size(), 2U + 600U + 4U);
    rtype::network::bytes::ByteWriter writer{wire};
    writeFields(writer, info, bytes, mapper);
    ASSERT_TRUE(writer.isValid());

    Letter read;
    rtype::network::bytes::ByteReader reader{wire};
    ASSERT_TRUE(readFields(reader, info, std::as_writable_bytes(std::span{&read, 1}), mapper));
    EXPECT_EQ(read.body, letter.body);
    EXPECT_EQ(read.stamp, 3);
}

TEST(ManifestHash, IsTheLayoutHashWithoutAList) {
    rtype::ecs::World world;
    static_cast<void>(world.getComponents().registerType<rtype::test::server::Position>());
    const auto& type = world.getComponents().getRegistry().find<rtype::test::server::Position>()->type;
    EXPECT_EQ(rtype::replication::serialization::computeManifestHash(type), type.layoutHash);
}

TEST(ManifestHash, DoesNotDependOnTheSizeOfAList) {
    rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<Letter>::makeTypeInfo();
    const std::uint64_t hash = rtype::replication::serialization::computeManifestHash(info);
    info.size += 8;  // as with another standard library, where `std::string` has another size
    info.fields.back().offset += 8;
    EXPECT_EQ(rtype::replication::serialization::computeManifestHash(info), hash);
}

TEST(ManifestHash, ChangesWithTheFields) {
    rtype::ecs::TypeInfo info = rtype::ecs::TypeTraits<Letter>::makeTypeInfo();
    const std::uint64_t hash = rtype::replication::serialization::computeManifestHash(info);
    info.fields.back().name = "other";
    EXPECT_NE(rtype::replication::serialization::computeManifestHash(info), hash);
}
