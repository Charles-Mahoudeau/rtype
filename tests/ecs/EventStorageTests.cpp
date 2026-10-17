/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventStorageTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>

#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/storage/EventStorage.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"

namespace {
/// @brief Number of Counter instances alive, to detect leaks and double destruction.
int liveCounters = 0;

/// @brief Counts its own instances.
struct Counter {
    Counter() noexcept { ++liveCounters; }
    ~Counter() { --liveCounters; }
    Counter(const Counter& /*other*/) noexcept { ++liveCounters; }
    Counter& operator=(const Counter& /*other*/) noexcept = default;
    Counter(Counter&& /*other*/) noexcept { ++liveCounters; }
    Counter& operator=(Counter&& /*other*/) noexcept = default;
};
}  // namespace

namespace rtype::test::event_storage {
struct Hit {
    std::int32_t damage;
};
RTYPE_ECS_EVENT(Hit, "test.storage.Hit", damage);

struct Tracked {
    std::int32_t value;
    Counter counter;
};
RTYPE_ECS_EVENT(Tracked, "test.storage.Tracked");

struct Packet {
    std::unique_ptr<std::int32_t> payload;
};
RTYPE_ECS_EVENT(Packet, "test.storage.Packet");

struct alignas(16) Aligned16 {
    std::array<float, 4> values;
};
RTYPE_ECS_EVENT(Aligned16, "test.storage.Aligned16", values);

struct GameOver {};
RTYPE_ECS_EVENT(GameOver, "test.storage.GameOver");
}  // namespace rtype::test::event_storage

namespace evs = rtype::test::event_storage;

namespace {
/// @return The description of `T`, kept alive for the whole test run: a storage only points to it.
template <typename T>
const rtype::ecs::EventInfo& infoOf() {
    static const rtype::ecs::EventInfo info = rtype::ecs::EventTraits<T>::makeInfo();
    return info;
}

template <typename T>
std::span<std::byte> bytesOf(T& value) {
    return std::as_writable_bytes(std::span{&value, 1});
}

/// @return The value of the event with this number.
template <typename T>
const T& eventAt(const rtype::ecs::storage::EventStorage& queue, rtype::ecs::EventSequence sequence) {
    return *rtype::ecs::storage::viewAs<T>(queue.get(sequence));
}

/// @brief Sends a Hit with this damage.
void sendHit(rtype::ecs::storage::EventStorage& queue, std::int32_t damage) {
    evs::Hit hit{.damage = damage};
    queue.sendMove(bytesOf(hit));
}
}  // namespace

TEST(EventStorage, StartsEmpty) {
    const rtype::ecs::storage::EventStorage queue{infoOf<evs::Hit>()};
    EXPECT_EQ(queue.getCount(), 0U);
    EXPECT_EQ(queue.getFirst(), 0U);
    EXPECT_EQ(queue.getEnd(), 0U);
    EXPECT_EQ(queue.getInfo().type.name, "test.storage.Hit");
}

TEST(EventStorage, NumbersEventsInTheOrderTheyAreSent) {
    rtype::ecs::storage::EventStorage queue{infoOf<evs::Hit>()};
    sendHit(queue, 10);
    sendHit(queue, 20);
    EXPECT_EQ(queue.getCount(), 2U);
    EXPECT_EQ(queue.getFirst(), 0U);
    EXPECT_EQ(queue.getEnd(), 2U);
    EXPECT_EQ(eventAt<evs::Hit>(queue, 0).damage, 10);
    EXPECT_EQ(eventAt<evs::Hit>(queue, 1).damage, 20);
}

TEST(EventStorage, KeepsEventsForOneMoreUpdate) {
    rtype::ecs::storage::EventStorage queue{infoOf<evs::Hit>()};
    sendHit(queue, 1);
    queue.update();
    sendHit(queue, 2);
    EXPECT_EQ(queue.getCount(), 2U);
    EXPECT_EQ(eventAt<evs::Hit>(queue, 0).damage, 1);
    EXPECT_EQ(eventAt<evs::Hit>(queue, 1).damage, 2);
    queue.update();
    EXPECT_EQ(queue.getCount(), 1U);
    EXPECT_EQ(queue.getFirst(), 1U);
    EXPECT_EQ(eventAt<evs::Hit>(queue, 1).damage, 2);
    queue.update();
    EXPECT_EQ(queue.getCount(), 0U);
    EXPECT_EQ(queue.getFirst(), 2U);
    EXPECT_EQ(queue.getEnd(), 2U);
}

TEST(EventStorage, ClearDropsEverythingAndKeepsTheNumbering) {
    rtype::ecs::storage::EventStorage queue{infoOf<evs::Hit>()};
    sendHit(queue, 1);
    queue.update();
    sendHit(queue, 2);
    queue.clear();
    EXPECT_EQ(queue.getCount(), 0U);
    EXPECT_EQ(queue.getFirst(), 2U);
    EXPECT_EQ(queue.getEnd(), 2U);
    sendHit(queue, 3);
    EXPECT_EQ(eventAt<evs::Hit>(queue, 2).damage, 3);
}

TEST(EventStorage, SendCopyLeavesTheSourceIntact) {
    rtype::ecs::storage::EventStorage queue{infoOf<evs::Hit>()};
    const evs::Hit hit{.damage = 7};
    queue.sendCopy(std::as_bytes(std::span{&hit, 1}));
    EXPECT_EQ(eventAt<evs::Hit>(queue, 0).damage, 7);
    EXPECT_EQ(hit.damage, 7);
}

TEST(EventStorage, StoresMoveOnlyEvents) {
    rtype::ecs::storage::EventStorage queue{infoOf<evs::Packet>()};
    evs::Packet packet{.payload = std::make_unique<std::int32_t>(42)};
    queue.sendMove(bytesOf(packet));
    EXPECT_EQ(packet.payload, nullptr);
    ASSERT_NE(eventAt<evs::Packet>(queue, 0).payload, nullptr);
    EXPECT_EQ(*eventAt<evs::Packet>(queue, 0).payload, 42);
}

TEST(EventStorage, DestroysEventsWhenDroppedClearedAndDestroyed) {
    liveCounters = 0;
    {
        rtype::ecs::storage::EventStorage queue{infoOf<evs::Tracked>()};
        for (std::int32_t index = 0; index < 3; ++index) {
            evs::Tracked event{.value = index, .counter = {}};
            queue.sendMove(bytesOf(event));
        }
        EXPECT_EQ(liveCounters, 3);
        queue.update();
        queue.update();
        EXPECT_EQ(liveCounters, 0);
        evs::Tracked kept{.value = 9, .counter = {}};
        queue.sendMove(bytesOf(kept));
        EXPECT_EQ(liveCounters, 2);  // `kept` and the queued copy
        queue.clear();
        EXPECT_EQ(liveCounters, 1);
        queue.sendMove(bytesOf(kept));
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(EventStorage, KeepsEventsAligned) {
    rtype::ecs::storage::EventStorage queue{infoOf<evs::Aligned16>()};
    for (int index = 0; index < 3; ++index) {
        evs::Aligned16 event{.values = {1.F, 2.F, 3.F, 4.F}};
        queue.sendMove(bytesOf(event));
    }
    queue.update();
    for (rtype::ecs::EventSequence sequence = queue.getFirst(); sequence < queue.getEnd(); ++sequence) {
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(queue.get(sequence).data()) % 16, 0U);
    }
}

TEST(EventStorage, CountsEventsWithoutData) {
    rtype::ecs::storage::EventStorage queue{infoOf<evs::GameOver>()};
    queue.sendMove({});
    queue.sendMove({});
    EXPECT_EQ(queue.getCount(), 2U);
    EXPECT_TRUE(queue.get(0).empty());
}

TEST(EventStorage, MoveTransfersTheEvents) {
    rtype::ecs::storage::EventStorage queue{infoOf<evs::Hit>()};
    sendHit(queue, 5);
    queue.update();
    const rtype::ecs::storage::EventStorage moved{std::move(queue)};
    EXPECT_EQ(moved.getCount(), 1U);
    EXPECT_EQ(eventAt<evs::Hit>(moved, 0).damage, 5);
}
