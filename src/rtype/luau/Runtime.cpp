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

/// @brief Tells whether @p flag is set in @p libs.
bool has(const rtype::luau::RuntimeConfig::Libs libs, const rtype::luau::RuntimeConfig::Libs flag) noexcept {
    return std::to_underlying(libs & flag) != 0;
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
}

Result<Bytecode> Runtime::compile(const std::string& source) const {
    std::size_t size = 0;
    lua_CompileOptions options = {
        .optimizationLevel = std::to_underlying(_config.optimizationLevel),
    };

    // NOLINTNEXTLINE(*-avoid-c-arrays)
    auto result = CPtr<char[]>{luau_compile(source.data(), source.size(), &options, &size)};

    if (size == 0) {
        return Failure{ErrorKind::kCompilation, std::format("unable to compile script: {}", result.get())};
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

Result<Script> Runtime::load(std::string name, const std::string& source) const {
    Result<Bytecode> bytecode = compile(source);

    if (!bytecode) {
        return forwardError<Script>(std::move(bytecode));
    }

    const std::int32_t result =
        luau_load(_state.get(), name.c_str(), bytecode->data().get(), bytecode->size(), kLuauGlobalEnv);

    if (result != 0) {
        const std::string message{lua_tostring(_state.get(), -1)};
        lua_pop(_state.get(), 1);
        return Failure{ErrorKind::kRuntime, std::format("unable to load script '{}': {}", name, message)};
    }

    Ref closure{*_state};
    Script script{std::move(closure), std::move(name), std::move(*bytecode)};
    return script;
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

Result<> Runtime::run(const Ref& closure) const {
    closure.push();
    if (lua_pcall(_state.get(), 0, 0, 0) != LUA_OK) {
        const std::string message{lua_tostring(_state.get(), -1)};
        lua_pop(_state.get(), 1);
        return Failure{ErrorKind::kRuntime, std::format("unable to run script: {}", message)};
    }
    return {};
}

Result<> Runtime::run(const Script& script) const { return run(script.closure()); }
}  // namespace rtype::luau
