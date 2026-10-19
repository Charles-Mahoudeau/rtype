/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** System
*/

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/system/ScriptCallable.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
namespace detail {
/// @brief The part of a system that knows its function's parameters: built once, then called at each run.
class SystemBody {
  public:
    SystemBody() = default;
    virtual ~SystemBody() = default;
    SystemBody(const SystemBody& other) = delete;
    SystemBody& operator=(const SystemBody& other) = delete;
    SystemBody(SystemBody&& other) = delete;
    SystemBody& operator=(SystemBody&& other) = delete;

    /// @brief Builds each parameter from `context` and calls the function.
    virtual void run(const SystemContext& context) = 0;
};

/// @brief The body of a system called with `Params...`: its callable (a function pointer, or a ScriptCallable's
/// callable) and the state of each parameter.
template <typename Callable, typename... Params>
class FunctionSystem final : public SystemBody {
  public:
    /// @brief Builds each parameter's state (registering what it needs; a configured parameter from its
    /// configuration, moved out of `configs`) and adds what it reads and writes to `access`.
    template <typename... Configs>
    FunctionSystem(World& world, Callable function, SystemAccess& access, std::tuple<Configs...>& configs);
    ~FunctionSystem() override = default;
    FunctionSystem(const FunctionSystem& other) = delete;
    FunctionSystem& operator=(const FunctionSystem& other) = delete;
    FunctionSystem(FunctionSystem&& other) = delete;
    FunctionSystem& operator=(FunctionSystem&& other) = delete;

    void run(const SystemContext& context) override;

  private:
    /// @brief Adds the access of every parameter.
    template <std::size_t... Indices>
    void addAccess(SystemAccess& access, std::index_sequence<Indices...> indices) const;

    /// @brief Calls the function with every parameter built for this run.
    template <std::size_t... Indices>
    void call(const SystemContext& context, std::index_sequence<Indices...> indices);

    Callable _function;                                          ///< The system's function.
    std::tuple<typename SystemParam<Params>::State...> _states;  ///< What each parameter keeps between runs.
};
}  // namespace detail

/// @brief A system: a function of the game's logic, with the parameters, access set, name and side deduced or given
/// when it is made.
///
/// @details It is made from a function (or a lambda without captures: a system keeps no hidden state, its data lives
/// in components and resources), or, for a script runtime only, from a ScriptCallable. Each parameter type says how to
/// build it from the world at each run (see SystemParam); together they give the system's access set, which the
/// scheduler orders systems by. run() builds the parameters and calls the function; the system's commands wait for
/// flushCommands(), which the scheduler calls after each phase. A system does not see its own changes: its next run
/// only reports what changed since this one. It keeps a pointer to the world: the world must outlive it.
/// @code
/// void movement(rtype::ecs::Query<Position, const Velocity> query, rtype::ecs::Res<Time> time) {
///     for (auto [entity, position, velocity] : query) {
///         position->x += velocity.x * time->fixedDelta;
///     }
/// }
///
/// rtype::ecs::System system{world, "rtype.movement", &movement};
/// system.setSide(rtype::ecs::Side::kBoth);
/// system.run();
/// system.flushCommands();
/// @endcode
class RTYPE_ECS_API System {
  public:
    /// @brief Makes a system of a function, deducing its parameters and their access.
    /// @details Fails to compile, with a message listing the accepted types, if a parameter is not one of
    /// `Query<Terms...>`, `Res<T>`, `ResMut<T>`, `NonSend<T>`, `NonSendMut<T>`, `Commands&`, `EventReader<T>`,
    /// `EventWriter<T>`, or a configured parameter. Also fails to compile, with a message, if a configured parameter
    /// has no configuration in `configs`, or a configuration is the `Config` of no parameter or of two.
    /// @param world The world it runs on; it must outlive the system.
    /// @param name Its stable name, such as "rtype.movement".
    /// @param configs The configurations of the configured parameters, one per parameter, in any order.
    template <typename... Params, typename... Configs>
    System(World& world, std::string name, void (*function)(Params...), Configs... configs);

    /// @brief Makes a system of a lambda without captures (converted to a function pointer). Any other callable
    /// fails to compile: a system keeps no hidden state (a script runtime uses ScriptCallable).
    template <typename Function, typename... Configs>
    System(World& world, std::string name, Function function, Configs... configs);

    /// @brief Makes a system of a callable that keeps state (a script runtime's), deducing its parameters from its
    /// call operator.
    /// @details Fails to compile, with a message, if the callable has no single call operator with explicit parameter
    /// types, or does not return void; the parameters and configurations are checked as for a function.
    template <typename Callable, typename... Configs>
    System(World& world, std::string name, ScriptCallable<Callable> script, Configs... configs);

    ~System() = default;
    System(const System& other) = delete;
    System& operator=(const System& other) = delete;
    System(System&& other) noexcept = default;
    System& operator=(System&& other) noexcept = default;

    /// @return The system's stable name.
    [[nodiscard]] std::string_view getName() const noexcept;

    /// @return Where the system runs; Side::kServer unless set.
    [[nodiscard]] Side getSide() const noexcept;

    /// @return What the system reads and writes, deduced from its parameters.
    [[nodiscard]] const SystemAccess& getAccess() const noexcept;

    /// @return The world's change tick at the system's last run.
    [[nodiscard]] Tick getLastRun() const noexcept;

    /// @brief Sets where the system runs.
    void setSide(Side side) noexcept;

    /// @brief Builds the parameters and calls the function, then moves the world's change tick forward.
    /// @details Values written during the run are stamped with the current tick; the next run only reports what
    /// changed after it, so a system does not see its own changes. If the function throws (a missing resource), the
    /// run does not count: the last-run tick stays as it was.
    /// @throws exceptions::MissingResourceException When a Res or ResMut parameter's resource has no value.
    void run();

    /// @brief Applies the commands the system recorded (Commands::flush()).
    void flushCommands();

  private:
    /// @return The body of a system calling `function` with `Params...`; only compiles when every parameter is
    /// accepted and every configured parameter has exactly its configuration in `configs`.
    template <typename Callable, typename... Params, typename... Configs>
    [[nodiscard]] static std::unique_ptr<detail::SystemBody> makeBody(World& world, Callable function,
                                                                      std::type_identity<void(Params...)> signature,
                                                                      SystemAccess& access, Configs... configs);

    /// @return The body of a system of a ScriptCallable's callable; only compiles when its call operator is single,
    /// has explicit parameter types and returns void.
    template <typename Callable, typename... Configs>
    [[nodiscard]] static std::unique_ptr<detail::SystemBody> makeScriptBody(World& world, Callable callable,
                                                                            SystemAccess& access, Configs... configs);

    /// @return `function` as a function pointer; only compiles for a lambda without captures.
    template <typename Function>
    [[nodiscard]] static auto toPointer(Function function);

    World* _world;                              ///< The world the system runs on.
    std::string _name;                          ///< Its stable name.
    Side _side{Side::kServer};                  ///< Where it runs.
    SystemAccess _access;                       ///< What it reads and writes; filled by the body when it is made.
    Commands _commands;                         ///< Its command buffer.
    Tick _lastRun;                              ///< The world's change tick at its last run.
    std::unique_ptr<detail::SystemBody> _body;  ///< Its function and the state of each parameter.
};
}  // namespace rtype::ecs

#include "System.tpp"
