/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Script
*/

#pragma once

#include <string>
#include <string_view>

#include "Bytecode.hpp"
#include "Ref.hpp"
#include "Result.hpp"

namespace rtype::luau {
/// @brief Script loaded by a `Runtime`.
///
/// Bundles the closure ready to be run, the name of the script and the bytecode it was loaded from.
/// Move-only. The closure belongs to the `lua_State` of the `Runtime` that loaded it, which must outlive the script.
class Script {
  public:
    /// @brief Creates a script from its parts.
    /// @param state Thread the closure was loaded in, where the script runs.
    /// @param thread Reference keeping @p state alive from the garbage collector.
    /// @param closure Reference to the loaded closure.
    /// @param name Name of the script.
    Script(lua_State* state, Ref thread, Ref closure, std::string name);

    /// @brief Releases the closure and thread references.
    ~Script() noexcept = default;

    /// @brief Copying is disabled: the closure reference and the bytecode have a single owner.
    Script(const Script&) noexcept = delete;

    /// @brief Copying is disabled: the closure reference and the bytecode have a single owner.
    Script& operator=(const Script&) noexcept = delete;

    /// @brief Transfers the closure, name and bytecode from another script.
    Script(Script&&) noexcept = default;

    /// @brief Transfers the thread, closure and name from another script, releasing the current ones.
    Script& operator=(Script&&) noexcept = default;

    /// @brief Gets the thread the script was loaded in.
    /// @return The Lua thread, valid as long as the script lives.
    [[nodiscard]] lua_State* state() const;

    /// @brief Gets the reference keeping the script's thread alive.
    /// @return The thread reference.
    [[nodiscard]] const Ref& thread() const;

    /// @brief Gets the reference to the loaded closure.
    /// @return The closure, to be passed to `Runtime::run`.
    [[nodiscard]] const Ref& closure() const;

    /// @brief Gets the name of the script.
    /// @return The name given at load time.
    [[nodiscard]] std::string_view name() const;

  private:
    lua_State* _state;  ///< Thread the closure was loaded in.
    Ref _thread;        ///< Keeps the thread alive from the garbage collector.
    Ref _closure;       ///< Loaded closure.
    std::string _name;  ///< Name of the script.
};
}  // namespace rtype::luau
