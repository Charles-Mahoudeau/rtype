/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DeletionQueueTests
*/

#include <gtest/gtest.h>

#include <stdexcept>
#include <utility>
#include <vector>

#include "render/vulkan/core/DeletionQueue.hpp"

namespace {

/// @brief Move-only stand-in for a RAII handle: records its id in a log when destroyed (moved-from ones excepted).
class Tracked {
  public:
    Tracked(int id, std::vector<int>& log) : _id{id}, _log{&log} {}
    ~Tracked() {
        if (_log != nullptr) {
            _log->push_back(_id);
        }
    }
    Tracked(const Tracked&) = delete;
    Tracked& operator=(const Tracked&) = delete;
    Tracked(Tracked&& other) noexcept : _id{other._id}, _log{std::exchange(other._log, nullptr)} {}
    Tracked& operator=(Tracked&&) = delete;

  private:
    int _id;                 ///< Recorded on destruction.
    std::vector<int>* _log;  ///< Where to record it; null once moved from.
};

}  // namespace

TEST(DeletionQueue, RejectsZeroFrames) {
    EXPECT_THROW(rtype::render::vulkan::core::DeletionQueue{0}, std::invalid_argument);
}

TEST(DeletionQueue, KeepsResourcesUntilTheirFrameIsFlushed) {
    std::vector<int> log;
    rtype::render::vulkan::core::DeletionQueue queue{2};
    queue.defer(0, Tracked{1, log});
    queue.defer(1, Tracked{2, log});
    EXPECT_TRUE(log.empty());
    EXPECT_EQ(queue.getPendingCount(0), 1U);

    queue.flush(0);
    EXPECT_EQ(log, (std::vector<int>{1}));
    EXPECT_EQ(queue.getPendingCount(0), 0U);
    EXPECT_EQ(queue.getPendingCount(1), 1U);
}

TEST(DeletionQueue, FlushesLastDeferredFirst) {
    std::vector<int> log;
    rtype::render::vulkan::core::DeletionQueue queue{1};
    queue.defer(0, Tracked{1, log});
    queue.deferCall(0, [&log] { log.push_back(2); });
    queue.defer(0, Tracked{3, log});

    queue.flush(0);
    EXPECT_EQ(log, (std::vector<int>{3, 2, 1}));
}

TEST(DeletionQueue, FlushAllEmptiesEveryFrame) {
    std::vector<int> log;
    rtype::render::vulkan::core::DeletionQueue queue{3};
    queue.defer(0, Tracked{1, log});
    queue.defer(2, Tracked{2, log});

    queue.flushAll();
    EXPECT_EQ(log.size(), 2U);
    for (std::size_t frame = 0; frame < queue.getFrameCount(); ++frame) {
        EXPECT_EQ(queue.getPendingCount(frame), 0U);
    }
}

TEST(DeletionQueue, DestructionFlushesPendingResources) {
    std::vector<int> log;
    {
        rtype::render::vulkan::core::DeletionQueue queue{2};
        queue.defer(1, Tracked{1, log});
    }
    EXPECT_EQ(log, (std::vector<int>{1}));
}

TEST(DeletionQueue, RejectsFramesOutOfRange) {
    std::vector<int> log;
    rtype::render::vulkan::core::DeletionQueue queue{2};
    EXPECT_THROW(queue.defer(2, Tracked{1, log}), std::out_of_range);
    EXPECT_THROW(queue.flush(2), std::out_of_range);
    EXPECT_THROW((void)queue.getPendingCount(2), std::out_of_range);
}
