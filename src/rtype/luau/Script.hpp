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

namespace rtype::luau {
/// @brief Script loaded by a `Runtime`.
///
/// Bundles the closure ready to be run, the name of the script and the bytecode it was loaded from.
/// Move-only. The closure belongs to the `lua_State` of the `Runtime` that loaded it, which must outlive the script.
class Script {
  public:
    /// @brief Creates a script from its parts.
    /// @param closure Reference to the loaded closure.
    /// @param name Name of the script.
    /// @param bytecode Bytecode the closure was loaded from.
    Script(Ref closure, std::string name, Bytecode bytecode);

    /// @brief Releases the closure reference and the bytecode.
    ~Script() noexcept = default;

    /// @brief Copying is disabled: the closure reference and the bytecode have a single owner.
    Script(const Script&) noexcept = delete;

    /// @brief Copying is disabled: the closure reference and the bytecode have a single owner.
    Script& operator=(const Script&) noexcept = delete;

    /// @brief Transfers the closure, name and bytecode from another script.
    Script(Script&&) noexcept = default;

    /// @brief Transfers the closure, name and bytecode from another script, releasing the current ones.
    Script& operator=(Script&&) noexcept = default;

    /// @brief Gets the reference to the loaded closure.
    /// @return The closure, to be passed to `Runtime::run`.
    [[nodiscard]] const Ref& closure() const;

    /// @brief Gets the name of the script.
    /// @return The name given at load time.
    [[nodiscard]] std::string_view name() const;

    /// @brief Gets the bytecode the closure was loaded from.
    /// @return The compiled bytecode.
    [[nodiscard]] const Bytecode& bytecode() const;

  private:
    Ref _closure;        ///< Loaded closure.
    std::string _name;   ///< Name of the script.
    Bytecode _bytecode;  ///< Bytecode the closure was loaded from.
};
}  // namespace rtype::luau
