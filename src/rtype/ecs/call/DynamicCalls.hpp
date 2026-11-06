/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicCalls
*/

#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
/// @brief The configuration of a DynamicCalls parameter: the engine calls the system uses, given when it is added.
/// @details They are numbered in this order (DynamicCalls::invoke(0, ...) calls the first).
struct CallList {
    std::vector<CallId> calls;  ///< The calls, registered before the system is added.
};

/// @brief A system parameter that calls engine calls chosen when the system is added, by id: for the calls a script
/// names.
///
/// @details Configured with a CallList (see SystemParam): the system gets a body of its own for every call of the list,
/// and its access set gets what each of them reads and writes, so the scheduler orders it as if it had the calls'
/// parameters itself; a call that uses a main-thread resource puts the system on the main thread. A call made through
/// it builds its parameters with the system's run (its ticks and its command buffer).
/// @code
/// void run(rtype::ecs::DynamicCalls calls) {
///     calls.invoke(0, arguments, result);
/// }
/// app.addSystem(rtype::ecs::Phase::kUpdate, "game.load", &run, rtype::ecs::CallList{{loadSoundId}});
/// @endcode
class RTYPE_ECS_API DynamicCalls {
  public:
    /// @param infos The calls of the system, in order; they must outlive the view.
    /// @param bodies The system's body of each call, in the same order; they must outlive the view.
    /// @param context The run the calls are made from.
    DynamicCalls(std::span<const CallInfo* const> infos, std::span<const std::unique_ptr<detail::CallBody>> bodies,
                 const SystemContext& context) noexcept;

    ~DynamicCalls() = default;
    DynamicCalls(const DynamicCalls& other) noexcept = default;
    DynamicCalls& operator=(const DynamicCalls& other) noexcept = default;
    DynamicCalls(DynamicCalls&& other) noexcept = default;
    DynamicCalls& operator=(DynamicCalls&& other) noexcept = default;

    /// @return The number of calls, as given in the CallList.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The call at `index`; `index` must be below getCount().
    [[nodiscard]] const CallInfo& getInfo(std::size_t index) const noexcept;

    /// @brief Calls the call at `index`; `index` must be below getCount().
    /// @param arguments The bytes of its arguments (CallInfo::arguments), empty when it takes none.
    /// @param result Where its result is written (CallInfo::result), empty when it returns none.
    /// @throws exceptions::CallException When `arguments` or `result` does not have the size of the call's
    /// description.
    /// @throws exceptions::MissingResourceException When a Res or ResMut parameter's resource has no value.
    void invoke(std::size_t index, std::span<const std::byte> arguments, std::span<std::byte> result) const;

  private:
    std::span<const CallInfo* const> _infos;                     ///< The system's calls.
    std::span<const std::unique_ptr<detail::CallBody>> _bodies;  ///< The system's body of each call.
    SystemContext _context;                                      ///< The run the calls are made from.
};

/// @brief `DynamicCalls`: configured with a CallList; the system keeps a body of each call.
template <>
struct RTYPE_ECS_API SystemParam<DynamicCalls> {
    using Config = CallList;  ///< The calls to use, given when the system is added.

    /// @brief The calls and the system's body of each.
    struct State {
        std::vector<const CallInfo*> infos;                     ///< The calls, in order.
        std::vector<std::unique_ptr<detail::CallBody>> bodies;  ///< The system's body of each call, in order.
    };

    /// @return The calls of `config`, each with a body of the system's own.
    /// @throws exceptions::CallException When an id is not a registered call.
    [[nodiscard]] static State init(World& world, Config config);

    /// @return A view of the calls for this run.
    [[nodiscard]] static DynamicCalls get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds what every call of the list reads and writes.
    static void addAccess(const State& state, SystemAccess& access);
};
}  // namespace rtype::ecs
