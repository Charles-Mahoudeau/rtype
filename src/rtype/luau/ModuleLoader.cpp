/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ModuleLoader
*/

#include "ModuleLoader.hpp"

#include <lua.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ErrorKind.hpp"
#include "Exceptions.hpp"
#include "Failure.hpp"
#include "ModuleSource.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "Runtime.hpp"
#include "Script.hpp"
#include "detail/RawState.hpp"
#include "detail/RequireFunction.hpp"

namespace rtype::luau {
namespace {
/// @brief Restores the height of a stack on destruction.
class StackGuard {
  public:
    explicit StackGuard(lua_State* state) noexcept : _state{state}, _top{lua_gettop(state)} {}
    ~StackGuard() noexcept { lua_settop(_state, _top); }
    StackGuard(const StackGuard&) = delete;
    StackGuard& operator=(const StackGuard&) = delete;
    StackGuard(StackGuard&&) = delete;
    StackGuard& operator=(StackGuard&&) = delete;

  private:
    lua_State* _state;  ///< Guarded state.
    int _top;           ///< Height to restore.
};

constexpr std::size_t kMaxDepth = 64;  ///< Modules being loaded at once, at most.

/// @brief Runs a function on destruction, whatever way the scope is left.
template <typename Function>
class Finally {
  public:
    explicit Finally(Function function) : _function{std::move(function)} {}
    ~Finally() { _function(); }
    Finally(const Finally&) = delete;
    Finally& operator=(const Finally&) = delete;
    Finally(Finally&&) = delete;
    Finally& operator=(Finally&&) = delete;

  private:
    Function _function;
};

using Edges = std::map<std::string, std::set<std::string>>;

/// @brief Reverses a graph: required key to the keys requiring it.
Edges reversed(const Edges& edges) {
    Edges result;
    for (const auto& [from, targets] : edges) {
        for (const auto& target : targets) {
            result[target].insert(from);
        }
    }
    return result;
}

/// @brief Collects @p roots and everything reachable from them in @p graph.
std::set<std::string> closure(const Edges& graph, const std::vector<std::string>& roots) {
    std::set<std::string> seen;
    std::vector<std::string> pending{roots};
    while (!pending.empty()) {
        const std::string current{std::move(pending.back())};
        pending.pop_back();
        if (!seen.insert(current).second) {
            continue;
        }
        if (const auto found = graph.find(current); found != graph.end()) {
            pending.insert(pending.end(), found->second.begin(), found->second.end());
        }
    }
    return seen;
}
}  // namespace

ModuleLoader::ModuleLoader(Runtime& runtime, Resolver resolver, RunHook runHook)
    : _runtime{runtime}, _resolver{std::move(resolver)}, _runHook{std::move(runHook)} {
    if (!_resolver) {
        throw exceptions::InvalidOperation{"a module loader needs a resolver"};
    }
    _token = detail::attachLoader(detail::rawState(_runtime), this);
}

ModuleLoader::~ModuleLoader() noexcept { detail::detachLoader(detail::rawState(_runtime), _token); }

Result<Ref> ModuleLoader::require(const std::string_view path) {
    ++_rootDepth;
    const Finally leave{[this] { --_rootDepth; }};
    Result<Ref> result = requireFrom({}, path);
    if (!result) {
        return Failure{ErrorKind::kRuntime, std::format("require(\"{}\"): {}", path, result.error().message())};
    }
    return result;
}

bool ModuleLoader::isLoaded(const std::string_view key) const { return _cache.contains(key); }

const Ref* ModuleLoader::value(const std::string_view key) const {
    const auto found = _cache.find(key);
    return found == _cache.end() ? nullptr : &found->second.value;
}

std::vector<std::string> ModuleLoader::loadedKeys() const {
    std::vector<std::string> keys;
    keys.reserve(_cache.size());
    for (const auto& [key, entry] : _cache) {
        keys.push_back(key);
    }
    return keys;
}

std::vector<std::string> ModuleLoader::dependents(const std::vector<std::string>& keys) const {
    const std::set<std::string> members = closure(reversed(_edges), keys);

    // Kahn's algorithm on the members, taking the smallest key among the ready ones.
    std::map<std::string, std::size_t> waiting;
    for (const auto& member : members) {
        std::size_t count = 0;
        if (const auto found = _edges.find(member); found != _edges.end()) {
            count = static_cast<std::size_t>(std::ranges::count_if(
                found->second, [&members](const std::string& target) { return members.contains(target); }));
        }
        waiting[member] = count;
    }
    const Edges users = reversed(_edges);
    std::set<std::string> ready;
    for (const auto& [member, count] : waiting) {
        if (count == 0) {
            ready.insert(member);
        }
    }
    std::vector<std::string> ordered;
    while (!ready.empty()) {
        std::string next = *ready.begin();
        ready.erase(ready.begin());
        if (const auto found = users.find(next); found != users.end()) {
            for (const auto& user : found->second) {
                if (members.contains(user) && --waiting[user] == 0) {
                    ready.insert(user);
                }
            }
        }
        ordered.push_back(std::move(next));
    }
    // Members of a cycle (a module requiring itself, say) never become ready: they are affected all the same.
    for (const auto& [member, count] : waiting) {
        if (count != 0 && !std::ranges::contains(ordered, member)) {
            ordered.push_back(member);
        }
    }
    return ordered;
}

std::vector<std::string> ModuleLoader::reachableFrom(const std::vector<std::string>& roots) const {
    const std::set<std::string> reached = closure(_edges, roots);
    return {reached.begin(), reached.end()};
}

void ModuleLoader::begin() {
    if (_inTransaction) {
        throw exceptions::InvalidOperation{"a module transaction is already active"};
    }
    _inTransaction = true;
    _transaction = Transaction{};
    _transaction.edges = _edges;
}

void ModuleLoader::setAside(const std::string& key) {
    if (const auto found = _cache.find(key); found != _cache.end()) {
        // What the transaction loaded itself is not brought back by a rollback: only the first entry is kept aside.
        if (!_transaction.added.contains(key)) {
            _transaction.aside.insert_or_assign(key, std::move(found->second));
        }
        _cache.erase(found);
    }
    _edges.erase(key);
}

void ModuleLoader::forget(const std::string& key) {
    _edges.erase(key);
    for (auto& [from, targets] : _edges) {
        targets.erase(key);
    }
}

void ModuleLoader::invalidate(const std::vector<std::string>& keys) {
    if (!_inTransaction) {
        throw exceptions::InvalidOperation{"invalidate needs an active module transaction"};
    }
    for (const auto& key : keys) {
        setAside(key);
    }
}

void ModuleLoader::commit() {
    if (!_inTransaction) {
        throw exceptions::InvalidOperation{"commit needs an active module transaction"};
    }
    _inTransaction = false;
    _transaction = Transaction{};
}

void ModuleLoader::rollback() {
    if (!_inTransaction) {
        throw exceptions::InvalidOperation{"rollback needs an active module transaction"};
    }
    for (const auto& key : _transaction.added) {
        _cache.erase(key);
    }
    for (auto& [key, entry] : _transaction.aside) {
        _cache.insert_or_assign(key, std::move(entry));
    }
    _edges = std::move(_transaction.edges);
    _inTransaction = false;
    _transaction = Transaction{};
}

void ModuleLoader::unload(const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        if (_inTransaction) {
            setAside(key);
            forget(key);
        } else {
            if (const auto found = _cache.find(key); found != _cache.end()) {
                _cache.erase(found);
            }
            forget(key);
        }
    }
}

bool ModuleLoader::isInTransaction() const noexcept { return _inTransaction; }

bool ModuleLoader::isLoading() const noexcept { return _rootDepth > 0 || _inTransaction || !_stack.empty(); }

Ref ModuleLoader::copyOf(const Ref& ref) const {
    lua_State* state = detail::rawState(_runtime);
    ref.push(state);
    return Ref::pop(state);
}

Result<Ref> ModuleLoader::requireFrom(const std::string_view requester, const std::string_view path) {
    Result<ModuleSource> resolved = _resolver(requester, path);
    if (!resolved) {
        return Failure{ErrorKind::kRuntime, std::string{resolved.error().message()}};
    }
    const ModuleSource& module = *resolved;
    if (module.key.empty()) {
        return Failure{ErrorKind::kRuntime, std::format("'{}' has an empty module key", module.chunkName)};
    }

    if (const auto cached = _cache.find(module.key); cached != _cache.end()) {
        if (!requester.empty()) {
            _edges[std::string{requester}].insert(module.key);
        }
        return copyOf(cached->second.value);
    }
    if (!isLoading()) {
        return Failure{ErrorKind::kRuntime,
                       std::format("'{}' is not loaded: require it at the top of a module", module.chunkName)};
    }
    const auto onStack = std::ranges::find(_stack, module.key, &Frame::key);
    if (onStack != _stack.end()) {
        std::string chain;
        for (const auto& frame : _stack) {
            chain += frame.chunkName + " -> ";
        }
        chain += module.chunkName;
        return Failure{ErrorKind::kRuntime, std::format("cycle: {}", chain)};
    }

    if (_stack.size() >= kMaxDepth) {
        return Failure{ErrorKind::kStackOverflow,
                       std::format("'{}' is too deep: modules require each other more than {} levels", module.chunkName,
                                   kMaxDepth)};
    }

    Result<Ref> loaded = execute(module);
    if (!loaded) {
        return loaded;
    }
    if (!requester.empty()) {
        _edges[std::string{requester}].insert(module.key);
    }
    return loaded;
}

Result<void> ModuleLoader::prepare(lua_State* thread, const Ref& closure, const std::string& key) {
    const StackGuard guard{thread};
    if (lua_checkstack(thread, 8) == 0) {
        return Failure{ErrorKind::kStackOverflow, "unable to prepare the module: the stack cannot grow"};
    }
    if (_runtime.isSandboxed()) {
        // The thread already has its own environment, falling back on the shared globals.
        lua_pushvalue(thread, LUA_GLOBALSINDEX);
    } else {
        // Threads share the globals: give the module a table of its own so its `require` stays its own.
        lua_newtable(thread);
        lua_newtable(thread);
        lua_pushvalue(thread, LUA_GLOBALSINDEX);
        lua_setfield(thread, -2, "__index");
        lua_setreadonly(thread, -1, 1);
        lua_setmetatable(thread, -2);
        closure.push(thread);
        lua_pushvalue(thread, -2);
        lua_setfenv(thread, -2);
        lua_pop(thread, 1);
    }
    detail::pushRequire(thread, _token, key);
    lua_setfield(thread, -2, "require");
    return {};
}

Result<Ref> ModuleLoader::execute(const ModuleSource& module) {
    Result<Script> script = _runtime.load(module.chunkName, module.source);
    if (!script) {
        return Failure{ErrorKind::kRuntime, std::format("failed: {}", script.error().message())};
    }
    lua_State* thread = script->state();
    if (auto prepared = prepare(thread, script->closure(), module.key); !prepared) {
        return Failure{ErrorKind::kRuntime, std::format("failed: {}", prepared.error().message())};
    }

    const StackGuard guard{thread};
    const int base = lua_gettop(thread);
    script->closure().push(thread);

    _stack.push_back({.key = module.key, .chunkName = module.chunkName});
    const Finally leave{[this] { _stack.pop_back(); }};
    if (_runHook) {
        _runHook(module.key, true);
    }
    const int status = lua_pcall(thread, 0, LUA_MULTRET, 0);
    if (_runHook) {
        _runHook(module.key, false);
    }

    if (status != LUA_OK) {
        const char* text = lua_tostring(thread, -1);
        std::string message{text != nullptr ? text : "unknown error"};
        _edges.erase(module.key);
        return Failure{ErrorKind::kRuntime, std::format("failed: {}", message)};
    }
    const int count = lua_gettop(thread) - base;
    if (count > 1) {
        _edges.erase(module.key);
        return Failure{ErrorKind::kRuntime, std::format("returned {} values", count)};
    }
    if (count == 0) {
        lua_pushnil(thread);
    } else if (lua_istable(thread, -1)) {
        lua_setreadonly(thread, -1, 1);
    }
    Ref value = Ref::pop(thread);
    Ref result = copyOf(value);

    _cache.insert_or_assign(module.key, Entry{.value = std::move(value), .chunkName = module.chunkName});
    if (_inTransaction) {
        _transaction.added.insert(module.key);
    }
    return result;
}
}  // namespace rtype::luau
