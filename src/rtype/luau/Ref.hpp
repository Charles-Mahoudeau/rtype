/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Ref
*/

#pragma once

#include <memory>

#include "Export.hpp"

struct lua_State;

namespace rtype::luau {
namespace detail {
class StateLife;
}  // namespace detail

/// @brief RAII handle on a Luau value stored in the registry.
///
/// Creates a registry reference (`lua_ref`) to a value of the Lua stack, keeping it alive
/// from the garbage collector, and releases it (`lua_unref`) on destruction. A `Ref` may outlive the `Runtime` that
/// owns its state: once the runtime is destroyed the `Ref` is no longer valid and never touches the closed state (a
/// state made without a `Runtime` is not watched, and must outlive its `Ref`s). A default-constructed or moved-from
/// `Ref`, or one whose runtime is gone, is empty: it references nothing and `isValid()` returns false. A `Ref` to nil
/// is valid: it takes no registry slot (Luau's `LUA_REFNIL`), `push()` pushes nil and `isNil()` returns true.
class RTYPE_LUAU_API Ref {
  public:
    /// @brief Registry reference id meaning "no reference" (Luau's `LUA_NOREF`).
    static constexpr int kLuaNoRef = -1;

    /// @brief Stack index of the value at the top of the Lua stack.
    static constexpr int kLuaStackTop = -1;

    /// @brief Creates a registry reference to the value at the top of the stack, then pops that value.
    /// @param state The Lua state owning the value. Its stack must not be empty.
    /// @return A Ref owning the registry reference to the popped value.
    /// @throws exceptions::InvalidState If @p state is null.
    [[nodiscard]] static Ref pop(lua_State* state);

    /// @brief Creates a registry reference to the value at the given stack index.
    ///
    /// The value is left on the stack.
    /// @param state The Lua state owning the value. The reference is stored against its main thread, so it
    ///        stays valid when @p state is a coroutine or a script thread.
    /// @param stackIndex Stack index of the value to reference (defaults to the top). It must be a valid
    ///        index of @p state's stack, not a pseudo-index.
    /// @throws exceptions::InvalidState If @p state is null.
    explicit Ref(lua_State* state, int stackIndex = kLuaStackTop);

    /// @brief Creates an empty Ref, which references nothing until a valid Ref is moved into it.
    Ref() = default;

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

    /// @brief Gives the Lua state holding the reference.
    /// @return The main thread of the Lua state the reference was created from, or nullptr if the Ref was
    ///         default-constructed.
    [[nodiscard]] lua_State* state() const noexcept;

    /// @brief Pushes the referenced value onto the top of the Lua stack.
    ///
    /// The reference itself is left untouched and stays valid after the call.
    /// @param state Lua state (or thread) onto whose stack the value is pushed. It must share its global state
    ///        with the state the reference was created from.
    /// @throws exceptions::InvalidRef If the Ref is empty.
    /// @throws exceptions::InvalidState If @p state is null.
    void push(lua_State* state) const;

    /// @brief Tells whether the Ref holds a registry reference of a state that is still open.
    /// @return False if the Ref is default-constructed or moved-from, or if its runtime was destroyed, true otherwise.
    [[nodiscard]] bool isValid() const noexcept;

    /// @brief Tells whether the Ref references nil.
    /// @return True if the referenced value is nil, false if it is another value or the Ref is empty.
    [[nodiscard]] bool isNil() const noexcept;

    /// @brief Tells whether the state of the Ref is still open.
    /// @return False once the `Runtime` that owns the state was destroyed; true for a `Ref` of a state no runtime
    ///         owns, and for an empty one.
    [[nodiscard]] bool isStateOpen() const noexcept;

  private:
    /// @brief Releases the registry reference, if the Ref holds one and its state is open.
    void release() noexcept;

    lua_State* _state{nullptr};                      ///< Main thread of the Lua state holding the reference.
    int _ref{kLuaNoRef};                             ///< Registry reference id.
    std::shared_ptr<const detail::StateLife> _life;  ///< Whether the state is open; null for a state of no runtime.
};
}  // namespace rtype::luau
