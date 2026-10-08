/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Ref
*/

#pragma once

#include <lua.h>

#include <functional>

namespace rtype::luau {
/// @brief RAII handle on a Luau value stored in the registry.
///
/// Creates a registry reference (`lua_ref`) to a value of the Lua stack, keeping it alive
/// from the garbage collector, and releases it (`lua_unref`) on destruction. The referenced
/// `lua_State` must outlive the `Ref`.
class Ref {
  public:
    /// @brief Creates a registry reference to the value at the given stack index.
    /// @param state The Lua state owning the value.
    /// @param stackIndex Stack index of the value to reference (defaults to the top).
    /// @param keepOnStack If false, the top of the stack is popped once the reference is created;
    ///                    if true, the stack is left untouched.
    explicit Ref(lua_State& state, int stackIndex = -1, bool keepOnStack = false);

    /// @brief Releases the registry reference.
    ~Ref();

    /// @brief Copy is deleted: a reference owns its registry slot.
    Ref(const Ref&) = delete;

    /// @brief Copy is deleted: a reference owns its registry slot.
    Ref& operator=(const Ref&) = delete;

    /// @brief Takes over the reference of another Ref, which is left empty.
    /// @param other The Ref to move from; it no longer references anything afterwards.
    Ref(Ref&& other) noexcept;

    /// @brief Releases the current reference, then takes over the reference of another Ref, which is left empty.
    /// @param other The Ref to move from; it no longer references anything afterwards.
    /// @return A reference to this Ref.
    Ref& operator=(Ref&& other) noexcept;

    /// @brief Pushes the referenced value onto the top of the Lua stack.
    ///
    /// The reference itself is left untouched and stays valid after the call.
    void push() const;

  private:
    std::reference_wrapper<lua_State> _state;  ///< Lua state holding the reference.
    int _ref;                                  ///< Registry reference id.
};
}  // namespace rtype::luau
