/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JoltRuntime
*/

#pragma once

#include <memory>

#include "rtype/physics/Export.hpp"

namespace JPH {
class Factory;
}  // namespace JPH

namespace rtype::physics::detail {

/// @brief Jolt's process-wide setup (allocator, factory, registered types), shared by every physics world.
///
/// @details Jolt keeps its factory and its registered types in globals, while a process runs several worlds (one per
/// room, each on its own thread). Each world holds a handle from acquire(): the first handle sets Jolt up, and
/// releasing the last one cleans it up, so a later acquire() sets it up again. Users never touch it: the physics
/// plugin's world takes the handle.
/// @code
/// const std::shared_ptr<const rtype::physics::detail::JoltRuntime> runtime =
///     rtype::physics::detail::JoltRuntime::acquire();
/// @endcode
class RTYPE_PHYSICS_API JoltRuntime {
  private:
    /// @brief Restricts construction to acquire(), which keeps Jolt set up at most once.
    struct Key {
        explicit Key() = default;
        ~Key() = default;
        Key(const Key& other) = default;
        Key& operator=(const Key& other) = default;
        Key(Key&& other) noexcept = default;
        Key& operator=(Key&& other) noexcept = default;
    };

  public:
    /// @brief Sets Jolt up. Only acquire() can call it.
    explicit JoltRuntime(Key key);
    /// @brief Cleans Jolt up.
    ~JoltRuntime();
    JoltRuntime(const JoltRuntime& other) = delete;
    JoltRuntime& operator=(const JoltRuntime& other) = delete;
    JoltRuntime(JoltRuntime&& other) noexcept = delete;
    JoltRuntime& operator=(JoltRuntime&& other) noexcept = delete;

    /// @brief Returns a handle to Jolt's setup, setting Jolt up if no handle is alive. Thread-safe.
    /// @return A handle that keeps Jolt set up while it lives; releasing the last one cleans Jolt up.
    [[nodiscard]] static std::shared_ptr<const JoltRuntime> acquire();

    /// @brief Whether Jolt is set up, as seen from rtype-physics.
    /// @details Jolt is linked statically: a binary that links it too has its own copy of Jolt's globals, so it must
    /// ask here instead of reading JPH::Factory::sInstance.
    /// @return True while a handle from acquire() is alive.
    [[nodiscard]] static bool isSetUp() noexcept;

  private:
    std::unique_ptr<JPH::Factory> _factory;  ///< Owns Jolt's factory; JPH::Factory::sInstance only points to it.
};

}  // namespace rtype::physics::detail
