/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceParam
*/

#pragma once

#include <cstdint>
#include <type_traits>

#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/resource/Resources.hpp"

namespace rtype::ecs {
/// @brief Whether a system parameter reads or writes its resource.
enum class ResourceAccess : std::uint8_t {
    kRead,   ///< Res, NonSend: the value is only read.
    kWrite,  ///< ResMut, NonSendMut: the value is written, and marked changed when it is.
};

/// @brief On which thread a system using the resource may run.
enum class ResourceThread : std::uint8_t {
    kAny,   ///< Res, ResMut: any thread.
    kMain,  ///< NonSend, NonSendMut: the main thread only (the platform, the renderer).
};

/// @brief A resource as a system parameter: Res, ResMut, NonSend and NonSendMut are its four aliases.
///
/// @details Made for each run from the world's resources, it gives the value (`->`, `*`, get()) and tells whether it
/// was added or changed since the system last ran. A written parameter marks the value changed only when it is written
/// through (`->`, `*`), as Mut does; get() never marks. The four aliases are distinct types, so a system's signature
/// tells the scheduler what it reads, what it writes, and whether it must run on the main thread.
/// @code
/// void movement(rtype::ecs::Query<Position, const Velocity> query, rtype::ecs::Res<Time> time);
/// void score(rtype::ecs::ResMut<Score> score) { score->value += 10; }
/// @endcode
/// @tparam A Read or write.
/// @tparam H Any thread, or the main thread only.
template <Resource T, ResourceAccess A, ResourceThread H>
class ResourceParam {
  public:
    using Type = T;  ///< The resource.
    /// @brief What `->` returns: `T*` for a written parameter, `const T*` for a read one.
    using Pointer = std::conditional_t<A == ResourceAccess::kWrite, T*, const T*>;
    /// @brief What `*` returns: `T&` for a written parameter, `const T&` for a read one.
    using Reference = std::conditional_t<A == ResourceAccess::kWrite, T&, const T&>;

    static constexpr ResourceAccess kAccess = A;  ///< Read or write.
    static constexpr ResourceThread kThread = H;  ///< Any thread, or the main thread only.

    /// @param resources The world's resources.
    /// @param lastRun Tick the system last ran at: isAdded() and isChanged() compare against it.
    /// @param thisRun The world's change tick now.
    /// @throws exceptions::MissingResourceException When the resource has no value (or is not registered).
    ResourceParam(Resources& resources, Tick lastRun, Tick thisRun);
    ~ResourceParam() = default;
    ResourceParam(const ResourceParam& other) = default;
    ResourceParam& operator=(const ResourceParam& other) = default;
    ResourceParam(ResourceParam&& other) noexcept = default;
    ResourceParam& operator=(ResourceParam&& other) noexcept = default;

    /// @return The value, read only: never marks it changed.
    [[nodiscard]] const T& get() const noexcept;

    /// @return The value; for a written parameter, marks it changed.
    [[nodiscard]] Pointer operator->() noexcept;

    /// @return The value; for a written parameter, marks it changed.
    [[nodiscard]] Reference operator*() noexcept;

    /// @return Whether the resource was inserted after the system last ran.
    [[nodiscard]] bool isAdded() const noexcept;

    /// @return Whether the resource was inserted or written after the system last ran (as of the start of the run).
    [[nodiscard]] bool isChanged() const noexcept;

  private:
    /// @brief How the value is held: through a Mut when written, a plain pointer when read.
    using Value = std::conditional_t<A == ResourceAccess::kWrite, Mut<T>, const T*>;

    /// @return The value, from the resources, as this parameter holds it.
    [[nodiscard]] static Value fetch(Resources& resources);

    Value _value;           ///< The value, in its slot.
    ComponentTicks _ticks;  ///< When the value was inserted and last written, as of the start of the run.
    Tick _lastRun;          ///< Tick the system last ran at.
    Tick _thisRun;          ///< The world's change tick when the run started.
};

/// @brief A resource the system reads.
template <Resource T>
using Res = ResourceParam<T, ResourceAccess::kRead, ResourceThread::kAny>;

/// @brief A resource the system writes.
template <Resource T>
using ResMut = ResourceParam<T, ResourceAccess::kWrite, ResourceThread::kAny>;

/// @brief A main-thread-only resource the system reads: the system runs on the main thread.
template <Resource T>
using NonSend = ResourceParam<T, ResourceAccess::kRead, ResourceThread::kMain>;

/// @brief A main-thread-only resource the system writes: the system runs on the main thread.
template <Resource T>
using NonSendMut = ResourceParam<T, ResourceAccess::kWrite, ResourceThread::kMain>;
}  // namespace rtype::ecs

#include "ResourceParam.tpp"
