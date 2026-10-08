/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#pragma once

#include <lua.h>

#include <filesystem>
#include <memory>
#include <string>

#include "Bytecode.hpp"
#include "Export.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "RuntimeConfig.hpp"
#include "Script.hpp"

struct lua_State;

namespace rtype::luau {
/// @brief Luau virtual machine: compiles, loads and runs scripts.
///
/// Owns a `lua_State` configured by a `RuntimeConfig`. Move-only: the state has a single owner.
class RTYPE_LUAU_API Runtime {
  public:
    /// @brief Creates a runtime and opens the libraries selected by the configuration.
    /// @param config Libraries to open and compilation options.
    /// @return The runtime, or an `ErrorKind::kUnknown` error if the Lua state cannot be created.
    [[nodiscard]] static Result<Runtime> create(RuntimeConfig config);

    /// @brief Closes the Lua state.
    ~Runtime() noexcept = default;

    /// @brief Copying is disabled: the Lua state has a single owner.
    Runtime(const Runtime& other) noexcept = delete;

    /// @brief Copying is disabled: the Lua state has a single owner.
    Runtime& operator=(const Runtime& other) noexcept = delete;

    /// @brief Transfers ownership of the Lua state from another runtime.
    Runtime(Runtime&& other) noexcept = default;

    /// @brief Transfers ownership of the Lua state from another runtime, closing the current one.
    Runtime& operator=(Runtime&& other) noexcept = default;

    /// @brief Compiles and loads a script from its source code, without running it.
    /// @param name Name of the script, used in error messages.
    /// @param source Luau source code.
    /// @return The loaded script, or an error (`ErrorKind::kCompilation` if the compilation fails,
    ///         `ErrorKind::kRuntime` if the bytecode cannot be loaded).
    [[nodiscard]] Result<Script> load(std::string name, const std::string& source) const;

    /// @brief Compiles and loads a script from a file, without running it.
    ///
    /// The script is named after the file name of @p path.
    /// @param path Path of the Luau source file.
    /// @return The loaded script, or an error if the file cannot be read, compiled or loaded.
    [[nodiscard]] Result<Script> load(const std::filesystem::path& path) const;

    /// @brief Runs a loaded closure in protected mode.
    /// @param closure Reference to the closure to call, without arguments.
    /// @return Success, or an `ErrorKind::kRuntime` error if the closure raised an error.
    [[nodiscard]] Result<> run(const Ref& closure) const;

    /// @brief Runs a loaded script in protected mode.
    /// @param script Script to run; it must have been loaded by this runtime.
    /// @return Success, or an `ErrorKind::kRuntime` error if the script raised an error.
    [[nodiscard]] Result<> run(const Script& script) const;

  private:
    /// @brief Deleter closing a `lua_State`.
    struct StateDeleter {
        /// @brief Closes the given Lua state.
        /// @param state State to close.
        void operator()(lua_State* state) const noexcept;
    };

    /// @brief Takes ownership of a Lua state and opens the configured libraries.
    /// @param state Lua state created with `luaL_newstate`.
    /// @param config Runtime configuration.
    /// @throws std::runtime_error If the library configuration is not supported.
    explicit Runtime(lua_State* state, RuntimeConfig config);

    /// @brief Compiles Luau source code to bytecode using the configured optimization level.
    /// @param source Luau source code.
    /// @return The bytecode, or an `ErrorKind::kCompilation` error.
    [[nodiscard]] Result<Bytecode> compile(const std::string& source) const;

    RuntimeConfig _config;                            ///< Configuration the runtime was created with.
    std::unique_ptr<lua_State, StateDeleter> _state;  ///< Owned Lua state.
};
}  // namespace rtype::luau
