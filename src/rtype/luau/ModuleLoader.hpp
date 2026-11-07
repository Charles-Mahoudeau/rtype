/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ModuleLoader
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "Export.hpp"
#include "ModuleSource.hpp"
#include "Ref.hpp"
#include "Result.hpp"

struct lua_State;

namespace rtype::luau {
class Runtime;

namespace detail {
int requireFunction(lua_State* state);
}  // namespace detail

/// @brief How the modules of a `ModuleLoader` relate to the shared globals (`ModuleLoader::Options`).
struct ModuleLoaderOptions {
    /// @brief When true, a global a module writes goes to the shared globals, so the other modules and the host
    ///        see it (the way scripts behaved before `require`); `require` itself stays per-module. It has no
    ///        effect on a sandboxed runtime, whose globals are read-only: there a module's writes stay in the
    ///        module. A bridge for games that still share data through globals.
    bool shareGlobals{false};
};

/// @brief Loads Luau modules by path (`require`), caching one instance per module.
///
/// Free of the engine: it knows neither files nor games. A `Resolver` callback turns a path into a `ModuleSource`; the
/// loader compiles it, runs it once in its own thread, checks what it returns and caches it by `ModuleSource::key`.
/// See `docs/luau/modules.md` for the full semantics.
///
/// Each module sees a `require` function bound to its own key, so relative paths resolve from the module that asks
/// and `pcall(require, ...)` works. The module has its own environment: the runtime's thread environment when the
/// runtime is sandboxed, otherwise a table of its own falling back on the shared globals (`__index`), so the `require`
/// of one module never leaks to another.
///
/// The runtime must outlive the loader, and the loader must not move (the modules hold it); a module's `require`
/// called after the loader is destroyed raises a Luau error.
class RTYPE_LUAU_API ModuleLoader {
  public:
    /// @brief Turns a require path into a module: `(requesterKey, path)`, with an empty requester key for a root
    ///        require. Returns the module or an error whose message is the reason shown to the script.
    using Resolver = std::function<Result<ModuleSource>(std::string_view requesterKey, std::string_view path)>;

    /// @brief Called around the execution of each module: `entering` is true before it runs and false after, also
    ///        when it fails. Calls are properly nested. Cache hits do not call it.
    using RunHook = std::function<void(std::string_view key, bool entering)>;

    /// @brief How the modules relate to the shared globals.
    using Options = ModuleLoaderOptions;

    /// @brief Creates a loader over a runtime.
    /// @param runtime Runtime the modules run in; it must outlive the loader.
    /// @param resolver Maps paths to module sources.
    /// @param runHook Optional hook called around each module run.
    /// @param options How the modules relate to the shared globals.
    ModuleLoader(Runtime& runtime, Resolver resolver, RunHook runHook = {}, Options options = {});

    /// @brief Detaches the loader from the `require` functions it gave to the modules.
    ~ModuleLoader() noexcept;

    ModuleLoader(const ModuleLoader&) = delete;
    ModuleLoader& operator=(const ModuleLoader&) = delete;
    ModuleLoader(ModuleLoader&&) = delete;
    ModuleLoader& operator=(ModuleLoader&&) = delete;

    /// @brief Requires a module from outside any script (a root require), loading it if needed.
    ///
    /// A module returning nothing gives nil. Errors read `require("<path>"): <reason>`.
    /// @param path Path given to the resolver, with an empty requester key.
    /// @return A reference to the module's value (nil for no value), or a `kRuntime` failure.
    [[nodiscard]] Result<Ref> require(std::string_view path);

    /// @brief Tells whether a module is in the cache.
    [[nodiscard]] bool isLoaded(std::string_view key) const;

    /// @brief Gets the cached value of a module.
    /// @return The value (a reference to nil for a module returning nothing), or nullptr if not loaded. Valid until
    ///         the cache changes.
    [[nodiscard]] const Ref* value(std::string_view key) const;

    /// @brief Lists the keys in the cache, sorted.
    [[nodiscard]] std::vector<std::string> loadedKeys() const;

    /// @brief Gets every module that depends, directly or not, on @p keys, plus @p keys themselves.
    /// @return Keys in topological order (dependencies first), ties broken by key.
    [[nodiscard]] std::vector<std::string> dependents(const std::vector<std::string>& keys) const;

    /// @brief Gets every module reachable from @p roots through the require graph, plus @p roots themselves.
    /// @return Keys, sorted.
    [[nodiscard]] std::vector<std::string> reachableFrom(const std::vector<std::string>& roots) const;

    /// @brief Starts a transaction (a hot reload): while it lasts, `require` may load new modules.
    /// @throws exceptions::InvalidOperation If a transaction is already active.
    void begin();

    /// @brief Takes the modules out of the cache and drops their own graph edges, keeping them for `rollback()`.
    /// @param keys Modules to invalidate; the ones not loaded are ignored.
    /// @throws exceptions::InvalidOperation If no transaction is active.
    void invalidate(const std::vector<std::string>& keys);

    /// @brief Ends the transaction, keeping the new state and dropping what was invalidated.
    /// @throws exceptions::InvalidOperation If no transaction is active.
    void commit();

    /// @brief Ends the transaction, restoring the cache and the graph exactly as they were at `begin()`.
    /// @throws exceptions::InvalidOperation If no transaction is active.
    void rollback();

    /// @brief Removes modules (the ones no longer reachable) from the cache and the graph.
    ///
    /// During a transaction it behaves like `invalidate()`, so `rollback()` brings them back.
    /// @param keys Modules to remove; the ones not loaded are ignored.
    void unload(const std::vector<std::string>& keys);

    /// @brief Tells whether a transaction is active.
    [[nodiscard]] bool isInTransaction() const noexcept;

  private:
    friend int detail::requireFunction(lua_State* state);

    /// @brief A module in the cache.
    struct Entry {
        Ref value;              ///< Value returned by the module.
        std::string chunkName;  ///< Name of the module in messages.
    };

    /// @brief Module being loaded.
    struct Frame {
        std::string key;
        std::string chunkName;
    };

    /// @brief What a transaction keeps to be able to roll back.
    struct Transaction {
        std::map<std::string, Entry> aside;                  ///< Entries taken out of the cache.
        std::set<std::string> added;                         ///< Keys cached during the transaction.
        std::map<std::string, std::set<std::string>> edges;  ///< The graph at `begin()`.
    };

    /// @brief Resolves, then gets or loads a module on behalf of @p requester. Errors carry the reason only.
    [[nodiscard]] Result<Ref> requireFrom(std::string_view requester, std::string_view path);

    /// @brief Compiles and runs a module, then caches it.
    [[nodiscard]] Result<Ref> execute(const ModuleSource& module);

    /// @brief Gives the module its environment: its own `require`.
    [[nodiscard]] Result<void> prepare(lua_State* thread, const Ref& closure, const std::string& key);

    /// @brief Makes a new reference to the same value.
    [[nodiscard]] Ref copyOf(const Ref& ref) const;

    /// @brief Tells whether a module may be loaded now (a root require or a transaction is going on).
    [[nodiscard]] bool isLoading() const noexcept;

    /// @brief Takes a module out of the cache and the graph, into the transaction.
    void setAside(const std::string& key);

    /// @brief Removes a module from the graph: its own edges and the ones leading to it.
    void forget(const std::string& key);

    Runtime& _runtime;                                    ///< Runtime the modules run in.
    Resolver _resolver;                                   ///< Maps paths to sources.
    RunHook _runHook;                                     ///< Called around each run.
    Options _options;                                     ///< How modules relate to the shared globals.
    std::map<std::string, Entry, std::less<>> _cache;     ///< Loaded modules by key.
    std::map<std::string, std::set<std::string>> _edges;  ///< Requester key to the keys it requires.
    std::vector<Frame> _stack;                            ///< Modules being loaded, outermost first.
    std::uint64_t _token{0};                              ///< Identifies this loader to its `require` functions.
    std::size_t _rootDepth{0};                            ///< Root requires in progress.
    bool _inTransaction{false};                           ///< Whether a transaction is active.
    Transaction _transaction;                             ///< State of the active transaction.
};
}  // namespace rtype::luau
