/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallInfo
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
class World;
struct SystemContext;

/// @brief Dense index of an engine call in its world's Calls, local to one world. Use the call's name across worlds.
using CallId = std::uint32_t;

/// @brief The id of no call.
inline constexpr CallId kInvalidCallId = std::numeric_limits<CallId>::max();

namespace detail {
/// @brief The part of an engine call that knows its function's parameters: the state of each parameter, and how to
/// call the function with them.
/// @details Each system that uses a call gets a body of its own (bind()), so the parameters keep their state per
/// system, as a system's own parameters do.
class CallBody {
  public:
    CallBody() = default;
    virtual ~CallBody() = default;
    CallBody(const CallBody& other) = delete;
    CallBody& operator=(const CallBody& other) = delete;
    CallBody(CallBody&& other) = delete;
    CallBody& operator=(CallBody&& other) = delete;

    /// @return A new body of the same function, whose parameters' states are built from `world`.
    [[nodiscard]] virtual std::unique_ptr<CallBody> bind(World& world) const = 0;

    /// @brief Adds what the function's parameters read and write to `access`.
    virtual void addAccess(SystemAccess& access) const = 0;

    /// @brief Builds each parameter from `context` and calls the function.
    /// @param arguments The bytes of the arguments (CallInfo::arguments), empty when the call takes none.
    /// @param result Where the result is written (CallInfo::result), empty when the call returns none.
    virtual void invoke(const SystemContext& context, std::span<const std::byte> arguments,
                        std::span<std::byte> result) = 0;
};
}  // namespace detail

/// @brief An engine call: a named function a plugin gives to scripts, with what it takes, what it returns and what it
/// reads and writes.
///
/// @details Its arguments and its result are described like ECS data (TypeInfo, declared with RTYPE_ECS_CALL_DATA), so
/// a script runtime can build and read them without knowing the C++ types, and `ecs` stays free of any script
/// language. Its access is the one of its function's parameters (Res, ResMut, NonSend...): a system that uses the
/// call (DynamicCalls) gets it added to its own, so the scheduler orders it as if it had those parameters.
struct CallInfo {
    CallId id{kInvalidCallId};               ///< Dense id, assigned by Calls.
    std::string name;                        ///< Stable, unique key: "rtype.audio.loadSound".
    std::optional<TypeInfo> arguments;       ///< What it takes; empty when it takes nothing.
    std::optional<TypeInfo> result;          ///< What it returns; empty when it returns nothing.
    SystemAccess access;                     ///< What it reads and writes.
    std::unique_ptr<detail::CallBody> body;  ///< Its function; used by Calls::invoke(), outside any system.
};
}  // namespace rtype::ecs
