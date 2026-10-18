/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemParam
*/

#pragma once

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryState.hpp"
#include "rtype/ecs/query/TermTraits.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
/// @brief What a system's parameters are built from at each run.
struct SystemContext {
    World* world;        ///< The world the system runs on.
    Commands* commands;  ///< The system's command buffer.
    Tick lastRun;        ///< Tick the system last ran at.
    Tick thisRun;        ///< The world's change tick now.
};

/// @brief How a system parameter type is made: what the system keeps for it between runs (`State`, built once by
/// `init()` when the system is made), how it is built for each run (`get()`), and what it reads and writes
/// (`addAccess()`).
///
/// @details Only the specializations below exist, one per accepted parameter type: `Query<Terms...>`, `Res<T>`,
/// `ResMut<T>`, `NonSend<T>`, `NonSendMut<T>` (all ResourceParam), `Commands&`, `EventReader<T>` and `EventWriter<T>`.
/// They are taken exactly as written: `Query` and the others by value, `Commands` by reference.
template <typename Param>
struct SystemParam;

/// @brief `Query<Terms...>`: the system keeps the QueryState; each run gets a view of it with the system's lastRun.
template <typename... Terms>
    requires ValidQuery<Terms...>
struct SystemParam<Query<Terms...>> {
    using State = QueryState<Terms...>;  ///< The query, built once (registers its components).

    /// @return The query, built from the world's components.
    [[nodiscard]] static State init(World& world);

    /// @return A view of the query for this run.
    [[nodiscard]] static Query<Terms...> get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds the components the query reads and writes.
    static void addAccess(const State& state, SystemAccess& access);
};

/// @brief `Res`, `ResMut`, `NonSend`, `NonSendMut`: the system keeps the resource's id (registered if needed, as
/// main-thread only for NonSend); each run requires the value.
template <Resource T, ResourceAccess A, ResourceThread H>
struct SystemParam<ResourceParam<T, A, H>> {
    using State = ResourceId;  ///< The resource's id.

    /// @return The resource's id, registering it if needed.
    [[nodiscard]] static State init(World& world);

    /// @return The resource for this run.
    /// @throws exceptions::MissingResourceException When the resource has no value.
    [[nodiscard]] static ResourceParam<T, A, H> get(State& state, const SystemContext& context);

    /// @brief Adds reading or writing the resource, and the main-thread need of NonSend.
    static void addAccess(const State& state, SystemAccess& access);
};

/// @brief `Commands&`: the system's own command buffer, applied when the system's commands are flushed.
template <>
struct RTYPE_ECS_API SystemParam<Commands&> {
    /// @brief Nothing to keep: the buffer belongs to the system.
    struct State {};

    /// @return Nothing.
    [[nodiscard]] static State init(World& world) noexcept;

    /// @return The system's buffer.
    [[nodiscard]] static Commands& get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds nothing: commands are structural changes applied at the flush, after the system ran.
    static void addAccess(const State& state, SystemAccess& access) noexcept;
};

/// @brief `EventReader<T>`: the system keeps its cursor in the queue of `T`, so it reads each event once.
template <Event T>
struct SystemParam<EventReader<T>> {
    /// @brief The event type's id, and where the system is in its queue.
    struct State {
        EventId id{0};       ///< The event type.
        EventCursor cursor;  ///< The next event the system has not read.
    };

    /// @return The event type's id (registering it if needed) and a cursor at the start of its queue.
    [[nodiscard]] static State init(World& world);

    /// @return The events not read yet; moves the cursor past them.
    [[nodiscard]] static EventReader<T> get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds reading the event type.
    static void addAccess(const State& state, SystemAccess& access);
};

/// @brief `EventWriter<T>`: the system keeps the event type's id.
template <Event T>
struct SystemParam<EventWriter<T>> {
    using State = EventId;  ///< The event type.

    /// @return The event type's id, registering it if needed.
    [[nodiscard]] static State init(World& world);

    /// @return A writer to the world's queue of `T`.
    [[nodiscard]] static EventWriter<T> get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds writing the event type.
    static void addAccess(const State& state, SystemAccess& access);
};

/// @brief A type a system may take as a parameter: `Query<Terms...>`, `Res<T>`, `ResMut<T>`, `NonSend<T>`,
/// `NonSendMut<T>`, `Commands&`, `EventReader<T>` or `EventWriter<T>`, exactly as written.
template <typename Param>
concept SystemParameter = requires { typename SystemParam<Param>::State; };
}  // namespace rtype::ecs

#include "SystemParam.tpp"
