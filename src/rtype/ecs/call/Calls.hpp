/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Calls
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <span>
#include <string_view>
#include <unordered_map>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/call/CallInfo.hpp"

namespace rtype::ecs {
class World;

/// @brief The engine calls of one world: the functions plugins give to scripts, by id and by name.
///
/// @details Plugins add their calls when they are built (App::addCall()); a script runtime then finds them by name,
/// builds their arguments from the descriptions, and calls them, either while its scripts load (invoke()) or from a
/// system that declared them (DynamicCalls). Registered calls never change, and pointers returned by the lookups stay
/// valid for the registry's lifetime (a deque keeps their addresses).
class RTYPE_ECS_API Calls {
  public:
    Calls() = default;
    ~Calls();
    Calls(const Calls& other) = delete;
    Calls& operator=(const Calls& other) = delete;
    Calls(Calls&& other) noexcept;
    Calls& operator=(Calls&& other) noexcept;

    /// @return The number of calls; ids go from 0 to this count minus one.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @brief Registers a call (see makeCallInfo() for one made of a C++ function).
    /// @param info The call; its id is ignored and set here. Its body must not be null.
    /// @return The call's id.
    /// @throws exceptions::CallException When the name is empty, already taken, or has the hash of another name, or
    /// when the call has no body.
    CallId add(CallInfo info);

    /// @return The call with this id, or nullptr.
    [[nodiscard]] const CallInfo* findById(CallId id) const noexcept;

    /// @return The call with this name, or nullptr.
    [[nodiscard]] const CallInfo* findByName(std::string_view name) const noexcept;

    /// @brief Calls a call once, outside any system: what a script runtime does while its scripts load.
    /// @details The function's parameters are built from `world` as for a system's first run, with a command buffer of
    /// their own, applied right after the call; the world's change tick then moves forward, as after a system's run.
    /// @param world The world the call belongs to.
    /// @param id The call.
    /// @param arguments The bytes of its arguments (CallInfo::arguments), empty when it takes none. Call data with a
    /// text or a list (a `std::string` or `std::vector` field) is read in place: give a CallBuffer of its type.
    /// @param result Where its result is written (CallInfo::result), empty when it returns none; a CallBuffer of its
    /// type too when it owns memory.
    /// @throws exceptions::CallException When no call has this id, or `arguments` or `result` does not have the size
    /// of the call's description, or is not aligned for call data that owns memory.
    /// @throws exceptions::MissingResourceException When a Res or ResMut parameter's resource has no value.
    void invoke(World& world, CallId id, std::span<const std::byte> arguments, std::span<std::byte> result);

  private:
    std::deque<CallInfo> _calls;                           ///< Calls by id; a deque keeps their addresses.
    std::unordered_map<std::uint64_t, CallId> _idsByHash;  ///< Ids by name hash.
};

/// @brief Checks the sizes of the bytes given to a call against its description.
/// @throws exceptions::CallException When `arguments` or `result` does not have the size the call expects, or holds
/// call data that owns memory and is not aligned for it.
RTYPE_ECS_API void checkCallBytes(const CallInfo& info, std::span<const std::byte> arguments,
                                  std::span<const std::byte> result);
}  // namespace rtype::ecs
