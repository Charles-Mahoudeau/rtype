/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ValuePack
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "Export.hpp"
#include "Value.hpp"

struct lua_State;

namespace rtype::luau {
/// @brief Ordered sequence of Values exchanged with a Lua stack, such as the arguments or the results of a call.
///
/// The runtime counterpart of a Luau type pack: `detail::pushValuePack()` hands every value over to a Lua state,
/// `detail::readValuePack()` reads a range of a Lua stack back into a pack, and `detail::popValuePack()` reads the
/// top of a Lua stack and pops it. `Function::call`
/// takes its arguments and returns its results as packs, and a `Function::Handler` returns its results as one.
class RTYPE_LUAU_API ValuePack {
  public:
    /// @brief Creates an empty pack.
    ValuePack() = default;

    /// @brief Destroys the pack.
    ~ValuePack() = default;

    /// @brief Copies another pack and its values.
    ValuePack(const ValuePack&) = default;

    /// @brief Copies another pack and its values.
    /// @return A reference to this pack.
    ValuePack& operator=(const ValuePack&) = default;

    /// @brief Takes over another pack.
    ValuePack(ValuePack&&) noexcept = default;

    /// @brief Takes over another pack.
    /// @return A reference to this pack.
    ValuePack& operator=(ValuePack&&) noexcept = default;

    /// @brief Gets the values held by the pack.
    /// @return A view over the values, in the order they were added.
    [[nodiscard]] std::span<const Value> values() const noexcept;

    /// @brief Gets the number of values held by the pack.
    /// @return The number of values added so far.
    [[nodiscard]] std::size_t size() const noexcept;

    /// @brief Appends a value after the ones already held.
    /// @param value The value to add.
    void add(Value value);

  private:
    std::vector<Value> _values;  ///< The values of the pack, in order.
};
}  // namespace rtype::luau
