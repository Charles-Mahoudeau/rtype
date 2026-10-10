/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Runtime
*/

#include "Runtime.hpp"

#include <lua.h>
#include <luacode.h>
#include <lualib.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "Bytecode.hpp"
#include "ErrorKind.hpp"
#include "Failure.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "Result.tpp"
#include "RuntimeConfig.hpp"
#include "Script.hpp"

namespace {
constexpr std::int32_t kLuauGlobalEnv = 0;

/// @brief Tells whether every library of @p flag is set in @p libs.
bool has(const rtype::luau::RuntimeConfig::Libs libs, const rtype::luau::RuntimeConfig::Libs flag) noexcept {
    return (libs & flag) == flag;
}

/// @brief Calls a closure in protected mode, without arguments or results.
/// @param state Lua state (or thread) to run the closure on.
/// @param closure Reference to the closure to call.
/// @return Success, or an `ErrorKind::kRuntime` error carrying the Lua error message.
rtype::luau::Result<> runClosure(lua_State* state, const rtype::luau::Ref& closure) {
    closure.push(state);
    if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
        const std::string message{lua_tostring(state, -1)};
        lua_pop(state, 1);
        return rtype::luau::Failure{rtype::luau::ErrorKind::kRuntime, std::format("unable to run script: {}", message)};
    }
    return {};
}
}  // namespace

namespace rtype::luau {
void Runtime::StateDeleter::operator()(lua_State* state) const noexcept { lua_close(state); }

Runtime::Runtime(lua_State* state, const RuntimeConfig config) : _config{config}, _state{state} {
    if (has(_config.libs, RuntimeConfig::Libs::kStandard)) {
        luaL_openlibs(_state.get());
    } else if (_config.libs != RuntimeConfig::Libs::kNone) {
        throw std::runtime_error{"unsupported library configuration"};
    }
    // TODO: enable later, this breaks more things than it fixes
    // luaL_sandbox(_state.get());
}

Result<Bytecode> Runtime::compile(const std::string& source) const {
    std::size_t size = 0;
    lua_CompileOptions options = {
        .optimizationLevel = std::to_underlying(_config.optimizationLevel),
    };

    // NOLINTNEXTLINE(*-avoid-c-arrays)
    auto result = CPtr<char[]>{luau_compile(source.data(), source.size(), &options, &size)};

    // On a compilation error, Luau returns a buffer made of a null byte followed by the error message.
    if (size == 0) {
        return Failure{ErrorKind::kCompilation, "unable to compile script"};
    }
    // NOLINTNEXTLINE(*-pro-bounds-avoid-unchecked-container-access)
    if (result[0] == 0) {
        // NOLINTNEXTLINE(*-pro-bounds-pointer-arithmetic)
        return Failure{ErrorKind::kCompilation, std::format("unable to compile script: {}", result.get() + 1)};
    }
    return Bytecode{std::move(result), size};
}

Result<Runtime> Runtime::create(const RuntimeConfig config) {
    lua_State* state = luaL_newstate();
    if (state == nullptr) {
        return Failure{ErrorKind::kUnknown, "unable to create lua state"};
    }
    try {
        return Runtime{state, config};
    } catch (const std::exception& e) {
        return Failure{ErrorKind::kUnknown, std::format("unable to create runtime: {}", e.what())};
    } catch (...) {
        return Failure{ErrorKind::kUnknown, "unable to create runtime"};
    }
}

lua_State* Runtime::state() const {
    std::cerr << "warning: direct access to lua state is an unsafe operation, this probably means that you are doing "
                 "something wrong or that an api is missing"
              << std::endl;
    return _state.get();
}

Result<Script> Runtime::load(std::string name, const std::string& source) const {
    Result<Bytecode> bytecode = compile(source);

    if (!bytecode) {
        return forwardError<Script>(std::move(bytecode));
    }

    lua_State* threadState = lua_newthread(_state.get());
    Ref thread = Ref::pop(_state.get());

    // TODO: enable later, when global state will be sandboxed
    // luaL_sandboxthread(threadState);

    const std::int32_t result =
        luau_load(threadState, name.c_str(), bytecode->data().get(), bytecode->size(), kLuauGlobalEnv);

    if (result != 0) {
        const std::string message{lua_tostring(threadState, -1)};
        lua_pop(threadState, 1);
        return Failure{ErrorKind::kRuntime, std::format("unable to load script '{}': {}", name, message)};
    }

    Ref closure = Ref::pop(threadState);
    return Script{threadState, std::move(thread), std::move(closure), std::move(name)};
}

Result<Script> Runtime::load(const std::filesystem::path& path) const {
    std::ifstream file{path, std::ios::binary};

    if (!file) {
        return Failure{ErrorKind::kUnknown, std::format("unable to open script '{}'", path.string())};
    }
    const std::string source{std::istreambuf_iterator{file}, std::istreambuf_iterator<char>{}};

    if (file.bad()) {
        return Failure{ErrorKind::kUnknown, std::format("unable to read script '{}'", path.string())};
    }
    return load(path.filename().string(), source);
}

Result<> Runtime::run(const Ref& closure) const { return runClosure(_state.get(), closure); }

// ReSharper disable once CppMemberFunctionMayBeStatic
// NOLINTNEXTLINE(*-convert-member-functions-to-static)
Result<> Runtime::run(const Script& script) const { return runClosure(script.state(), script.closure()); }

Ref Runtime::global(const std::string& name) const {
    if (lua_getglobal(_state.get(), name.c_str()) == LUA_TNIL) {
        std::cerr << "warning: global '" << name << "' not found\n";
    }
    return Ref::pop(_state.get());
}
}  // namespace rtype::luau
