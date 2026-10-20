/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JoltRuntimeTests
*/

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

#include "rtype/physics/detail/JoltRuntime.hpp"

using rtype::physics::detail::JoltRuntime;

namespace {
constexpr std::size_t kThreadCount = 8;
constexpr std::size_t kAcquisitionsPerThread = 200;
}  // namespace

TEST(JoltRuntime, NoHandleMeansJoltIsNotSetUp) { EXPECT_FALSE(JoltRuntime::isSetUp()); }

TEST(JoltRuntime, AcquireSetsJoltUp) {
    const std::shared_ptr<const JoltRuntime> runtime = JoltRuntime::acquire();
    ASSERT_NE(runtime, nullptr);
    EXPECT_TRUE(JoltRuntime::isSetUp());
}

TEST(JoltRuntime, HandlesShareOneRuntime) {
    const std::shared_ptr<const JoltRuntime> first = JoltRuntime::acquire();
    const std::shared_ptr<const JoltRuntime> second = JoltRuntime::acquire();
    EXPECT_EQ(first.get(), second.get());
}

TEST(JoltRuntime, ReleasingTheLastHandleCleansJoltUp) {
    std::shared_ptr<const JoltRuntime> first = JoltRuntime::acquire();
    std::shared_ptr<const JoltRuntime> second = JoltRuntime::acquire();
    first.reset();
    EXPECT_TRUE(JoltRuntime::isSetUp());
    second.reset();
    EXPECT_FALSE(JoltRuntime::isSetUp());
}

TEST(JoltRuntime, CopiesOfAHandleKeepJoltSetUp) {
    std::shared_ptr<const JoltRuntime> handle = JoltRuntime::acquire();
    const std::shared_ptr<const JoltRuntime> copy = handle;
    handle.reset();
    EXPECT_TRUE(JoltRuntime::isSetUp());
}

TEST(JoltRuntime, AcquireAfterAFullReleaseSetsJoltUpAgain) {
    JoltRuntime::acquire().reset();
    ASSERT_FALSE(JoltRuntime::isSetUp());
    const std::shared_ptr<const JoltRuntime> runtime = JoltRuntime::acquire();
    ASSERT_NE(runtime, nullptr);
    EXPECT_TRUE(JoltRuntime::isSetUp());
}

TEST(JoltRuntime, ConcurrentAcquisitionsKeepJoltSetUpWhileAHandleLives) {
    std::atomic<std::size_t> notSetUp{0};
    {
        std::vector<std::jthread> threads;
        threads.reserve(kThreadCount);
        for (std::size_t thread = 0; thread < kThreadCount; ++thread) {
            threads.emplace_back([&notSetUp] {
                for (std::size_t acquisition = 0; acquisition < kAcquisitionsPerThread; ++acquisition) {
                    const std::shared_ptr<const JoltRuntime> runtime = JoltRuntime::acquire();
                    if (!JoltRuntime::isSetUp()) {
                        ++notSetUp;
                    }
                }
            });
        }
    }
    EXPECT_EQ(notSetUp.load(), 0U);
    EXPECT_FALSE(JoltRuntime::isSetUp());
}
