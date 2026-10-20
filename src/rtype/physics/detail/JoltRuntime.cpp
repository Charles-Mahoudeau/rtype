/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JoltRuntime
*/

#include "JoltRuntime.hpp"

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/RegisterTypes.h>

#include <cstddef>
#include <memory>
#include <mutex>

namespace rtype::physics::detail {

namespace {

/// @brief The runtime shared by the handles, and how many handles are alive.
struct SharedRuntime {
    std::mutex mutex;                      ///< Guards the two members below.
    std::size_t handles = 0;               ///< Handles alive.
    std::unique_ptr<JoltRuntime> runtime;  ///< Alive while handles > 0.
};

/// @return The process's only SharedRuntime, built on first use.
SharedRuntime& sharedRuntime() {
    static SharedRuntime shared;
    return shared;
}

}  // namespace

JoltRuntime::JoltRuntime([[maybe_unused]] Key key) {
    JPH::RegisterDefaultAllocator();
    _factory = std::make_unique<JPH::Factory>();
    JPH::Factory::sInstance = _factory.get();
    JPH::RegisterTypes();
}

JoltRuntime::~JoltRuntime() {
    JPH::UnregisterTypes();
    JPH::Factory::sInstance = nullptr;
}

bool JoltRuntime::isSetUp() noexcept { return JPH::Factory::sInstance != nullptr; }

std::shared_ptr<const JoltRuntime> JoltRuntime::acquire() {
    SharedRuntime& shared = sharedRuntime();
    const std::scoped_lock lock{shared.mutex};

    if (shared.handles == 0) {
        shared.runtime = std::make_unique<JoltRuntime>(Key{});
    }
    ++shared.handles;
    return {shared.runtime.get(), [](const JoltRuntime* /*runtime*/) {
                SharedRuntime& owner = sharedRuntime();
                const std::scoped_lock releaseLock{owner.mutex};
                if (--owner.handles == 0) {
                    owner.runtime.reset();
                }
            }};
}

}  // namespace rtype::physics::detail
