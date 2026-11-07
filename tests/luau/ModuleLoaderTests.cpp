/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ModuleLoaderTests
*/

#include <gtest/gtest.h>
#include <lua.h>
#include <lualib.h>

#include <algorithm>
#include <format>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Failure.hpp"
#include "rtype/luau/ModuleLoader.hpp"
#include "rtype/luau/ModuleSource.hpp"
#include "rtype/luau/Ref.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/detail/RawState.hpp"

namespace {
using rtype::luau::ErrorKind;
using rtype::luau::ModuleLoader;
using rtype::luau::ModuleSource;
using rtype::luau::Ref;
using rtype::luau::Result;
using Keys = std::vector<std::string>;

/// @brief A runtime, an in-memory set of modules (key to source) and a loader over them.
///
/// Paths: `./name` and `name` give the module `name`; `alias` gives `a`; anything else is not found. The chunk name
/// of a module is `<key>.luau`.
class Modules {
  public:
    explicit Modules(const bool sandboxed = true) : _runtime{makeRuntime()} {
        if (sandboxed) {
            _runtime.sandbox();
        }
        _loader = std::make_unique<ModuleLoader>(
            _runtime, [this](std::string_view requester, std::string_view path) { return resolve(requester, path); },
            [this](std::string_view key, bool entering) {
                events.push_back(std::format("{}{}", entering ? "+" : "-", key));
            });
    }

    ~Modules() = default;
    Modules(const Modules&) = delete;
    Modules& operator=(const Modules&) = delete;
    Modules(Modules&&) = delete;
    Modules& operator=(Modules&&) = delete;

    ModuleLoader& loader() { return *_loader; }
    void dropLoader() { _loader.reset(); }
    rtype::luau::Runtime& runtime() { return _runtime; }

    /// @brief Requires a path from the root and fails the test if it does not load.
    Ref require(const std::string& path) {
        auto result = _loader->require(path);
        EXPECT_TRUE(result) << (result ? "" : std::string{result.error().message()});
        return result ? std::move(*result) : Ref{};
    }

    /// @brief The error of a root require that must fail.
    std::string requireError(const std::string& path) {
        auto result = _loader->require(path);
        EXPECT_FALSE(result);
        return result ? std::string{} : std::string{result.error().message()};
    }

    std::string typeOf(const Ref& ref) {
        lua_State* state = rtype::luau::detail::rawState(_runtime);
        ref.push(state);
        std::string name{luaL_typename(state, -1)};
        lua_pop(state, 1);
        return name;
    }

    std::string text(const Ref& ref) {
        lua_State* state = rtype::luau::detail::rawState(_runtime);
        ref.push(state);
        const char* value = lua_tostring(state, -1);
        std::string result{value != nullptr ? value : ""};
        lua_pop(state, 1);
        return result;
    }

    /// @brief Calls a function value without arguments and gives its first result as text, or `error: <message>`.
    std::string call(const Ref& function) {
        lua_State* state = rtype::luau::detail::rawState(_runtime);
        function.push(state);
        const int status = lua_pcall(state, 0, 1, 0);
        const char* value = lua_tostring(state, -1);
        std::string result{value != nullptr ? value : typeNameAtTop(state)};
        lua_pop(state, 1);
        return status == LUA_OK ? result : "error: " + result;
    }

    std::map<std::string, std::string> sources;  ///< Module key to source.
    std::vector<std::string> events;             ///< Run hook calls, `+key` entering and `-key` leaving.
    std::vector<std::string> requesters;         ///< Requester key of each resolver call.

  private:
    static rtype::luau::Runtime makeRuntime() {
        auto runtime = rtype::luau::Runtime::create(
            rtype::luau::RuntimeConfig{.libs = rtype::luau::RuntimeConfig::Libs::kStandard});
        EXPECT_TRUE(runtime);
        return std::move(*runtime);
    }

    static std::string typeNameAtTop(lua_State* state) { return luaL_typename(state, -1); }

    Result<ModuleSource> resolve(const std::string_view requester, std::string_view path) {
        requesters.emplace_back(requester);
        if (path == "alias") {
            path = "a";
        }
        if (path.starts_with("./")) {
            path.remove_prefix(2);
        }
        const auto found = sources.find(std::string{path});
        if (found == sources.end()) {
            return rtype::luau::Failure{ErrorKind::kRuntime, std::format("not found: {}", path)};
        }
        return ModuleSource{.key = found->first, .chunkName = found->first + ".luau", .source = found->second};
    }

    rtype::luau::Runtime _runtime;
    std::unique_ptr<ModuleLoader> _loader;
};

TEST(ModuleLoader, ReturnsTheValueOfAModule) {
    Modules modules;
    modules.sources["a"] = "return 42";

    EXPECT_EQ(modules.text(modules.require("a")), "42");
    EXPECT_TRUE(modules.loader().isLoaded("a"));
}

TEST(ModuleLoader, RunsAModuleOnceAndCachesIt) {
    Modules modules;
    modules.sources["a"] = "return {}";
    modules.sources["b"] = "return require('./a')";

    const Ref first = modules.require("a");
    const Ref second = modules.require("./a");
    const Ref third = modules.require("b");

    EXPECT_EQ(modules.events, (Keys{"+a", "-a", "+b", "-b"}));
    lua_State* state = rtype::luau::detail::rawState(modules.runtime());
    first.push(state);
    second.push(state);
    third.push(state);
    EXPECT_TRUE(lua_rawequal(state, -3, -2));
    EXPECT_TRUE(lua_rawequal(state, -3, -1));
    lua_pop(state, 3);
}

TEST(ModuleLoader, PathsWithTheSameKeyShareOneInstance) {
    Modules modules;
    modules.sources["a"] = "count = (count or 0) + 1; return {}";

    const Ref byName = modules.require("a");
    const Ref byAlias = modules.require("alias");

    EXPECT_EQ(modules.events, (Keys{"+a", "-a"}));
    lua_State* state = rtype::luau::detail::rawState(modules.runtime());
    byName.push(state);
    byAlias.push(state);
    EXPECT_TRUE(lua_rawequal(state, -2, -1));
    lua_pop(state, 2);
}

TEST(ModuleLoader, FreezesAReturnedTableShallowly) {
    Modules modules;
    modules.sources["a"] = "return {value = 1, inner = {n = 1}}";

    const Ref table = modules.require("a");

    EXPECT_EQ(modules.typeOf(table), "table");
    lua_State* state = rtype::luau::detail::rawState(modules.runtime());
    table.push(state);
    EXPECT_TRUE(lua_getreadonly(state, -1));
    lua_getfield(state, -1, "inner");
    EXPECT_FALSE(lua_getreadonly(state, -1));
    lua_pop(state, 2);
}

TEST(ModuleLoader, AFrozenTableRejectsWritesFromModulesButNotItsNestedTables) {
    Modules modules;
    modules.sources["a"] = "return {value = 1, inner = {n = 1}}";
    modules.sources["b"] =
        "local a = require('a'); local ok = pcall(function() a.value = 2 end); "
        "a.inner.n = 2; return tostring(ok) .. a.value .. a.inner.n";

    EXPECT_EQ(modules.text(modules.require("b")), "false12");
}

TEST(ModuleLoader, ANonTableValuePassesThrough) {
    Modules modules;
    modules.sources["n"] = "return 7";
    modules.sources["s"] = "return 'text'";
    modules.sources["f"] = "return function() return 'called' end";

    EXPECT_EQ(modules.typeOf(modules.require("n")), "number");
    EXPECT_EQ(modules.text(modules.require("s")), "text");
    EXPECT_EQ(modules.call(modules.require("f")), "called");
}

TEST(ModuleLoader, AModuleReturningNothingGivesNilAndIsCached) {
    Modules modules;
    modules.sources["a"] = "x = 1";
    modules.sources["b"] = "local a = require('a'); local c = require('a'); return a == nil and c == nil";

    EXPECT_EQ(modules.typeOf(modules.require("a")), "nil");
    EXPECT_TRUE(modules.loader().isLoaded("a"));
    EXPECT_EQ(modules.typeOf(modules.require("b")), "boolean");
    EXPECT_EQ(modules.events, (Keys{"+a", "-a", "+b", "-b"}));
}

TEST(ModuleLoader, TwoValuesAreAnErrorNamingTheRequesterAndTheLine) {
    Modules modules;
    modules.sources["two"] = "return 1, 2";
    modules.sources["main"] = "local x = 1\nlocal two = require('two')";

    const std::string error = modules.requireError("main");

    EXPECT_EQ(error, "require(\"main\"): failed: main.luau:2: require(\"two\"): returned 2 values");
    EXPECT_FALSE(modules.loader().isLoaded("two"));
}

TEST(ModuleLoader, ARootRequireOfTwoValuesIsAnError) {
    Modules modules;
    modules.sources["two"] = "return 1, 2";

    EXPECT_EQ(modules.requireError("two"), "require(\"two\"): returned 2 values");
}

TEST(ModuleLoader, ARootRequireMayGetNothing) {
    Modules modules;
    modules.sources["entry"] = "local x = 1";

    const auto result = modules.loader().require("entry");

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->isNil());
}

TEST(ModuleLoader, ACycleShowsTheChain) {
    Modules modules;
    modules.sources["a"] = "require('b')";
    modules.sources["b"] = "require('a')";

    const std::string error = modules.requireError("a");

    EXPECT_EQ(error,
              "require(\"a\"): failed: a.luau:1: require(\"b\"): "
              "failed: b.luau:1: require(\"a\"): cycle: a.luau -> b.luau -> a.luau");
    EXPECT_EQ(modules.events, (Keys{"+a", "+b", "-b", "-a"}));
}

TEST(ModuleLoader, ASelfRequireIsACycle) {
    Modules modules;
    modules.sources["a"] = "require('a')";

    EXPECT_NE(modules.requireError("a").find("cycle: a.luau -> a.luau"), std::string::npos);
}

TEST(ModuleLoader, AResolverErrorIsTheReason) {
    Modules modules;
    modules.sources["a"] = "\nrequire('missing')";

    EXPECT_EQ(modules.requireError("missing"), "require(\"missing\"): not found: missing");
    EXPECT_EQ(modules.requireError("a"), "require(\"a\"): failed: a.luau:2: require(\"missing\"): not found: missing");
}

TEST(ModuleLoader, AnInnerErrorIsWrapped) {
    Modules modules;
    modules.sources["bad"] = "error('boom', 0)";
    modules.sources["a"] = "require('bad')";

    EXPECT_EQ(modules.requireError("a"), "require(\"a\"): failed: a.luau:1: require(\"bad\"): failed: boom");
}

TEST(ModuleLoader, ASyntaxErrorIsAFailure) {
    Modules modules;
    modules.sources["a"] = "local = =";

    const auto result = modules.loader().require("a");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), ErrorKind::kRuntime);
    EXPECT_NE(std::string{result.error().message()}.find("failed:"), std::string::npos);
}

TEST(ModuleLoader, AFailedModuleIsNotCachedAndALaterRequireRetries) {
    Modules modules;
    modules.sources["a"] = "error('first')";

    EXPECT_FALSE(modules.loader().require("a"));
    EXPECT_FALSE(modules.loader().isLoaded("a"));

    modules.sources["a"] = "return 'second'";
    EXPECT_EQ(modules.text(modules.require("a")), "second");
}

TEST(ModuleLoader, ANonStringArgumentIsAnError) {
    Modules modules;
    modules.sources["a"] = "require(5)";
    modules.sources["b"] = "require()";

    EXPECT_NE(modules.requireError("a").find("a.luau:1: require expects a string path, got number"), std::string::npos);
    EXPECT_NE(modules.requireError("b").find("require expects a string path, got no value"), std::string::npos);
}

TEST(ModuleLoader, EachModuleHasItsOwnGlobalsInASandboxedRuntime) {
    Modules modules{true};
    modules.sources["a"] = "shared = 1; return shared";
    modules.sources["b"] = "require('a'); return shared == nil";

    modules.require("a");
    EXPECT_EQ(modules.text(modules.require("b")), "");
    EXPECT_EQ(modules.typeOf(modules.require("b")), "boolean");
    modules.sources["c"] = "require('a'); assert(shared == nil)";
    modules.require("c");
}

TEST(ModuleLoader, EachModuleHasItsOwnGlobalsInAnUnsandboxedRuntimeToo) {
    Modules modules{false};
    modules.sources["a"] = "shared = 1; return shared";
    modules.sources["b"] = "require('a'); assert(shared == nil); return type(require)";

    EXPECT_FALSE(modules.runtime().isSandboxed());
    modules.require("a");
    EXPECT_EQ(modules.text(modules.require("b")), "function");
    lua_State* state = rtype::luau::detail::rawState(modules.runtime());
    lua_getglobal(state, "shared");
    EXPECT_TRUE(lua_isnil(state, -1));
    lua_getglobal(state, "require");
    EXPECT_TRUE(lua_isnil(state, -1));
    lua_pop(state, 2);
}

TEST(ModuleLoader, ModulesSeeTheSharedGlobals) {
    Modules modules{false};
    modules.sources["a"] = "return answer";
    lua_State* state = rtype::luau::detail::rawState(modules.runtime());
    lua_pushnumber(state, 42);
    lua_setglobal(state, "answer");

    EXPECT_EQ(modules.text(modules.require("a")), "42");
}

TEST(ModuleLoader, RequireWorksThroughPcall) {
    Modules modules;
    modules.sources["a"] = "return 'ok'";
    modules.sources["main"] =
        "local ok, value = pcall(require, 'a'); local bad, err = pcall(require, 'nope'); "
        "return tostring(ok) .. ':' .. value .. ':' .. tostring(bad) .. ':' .. err";

    EXPECT_EQ(modules.text(modules.require("main")), "true:ok:false:require(\"nope\"): not found: nope");
}

TEST(ModuleLoader, TheResolverGetsTheKeyOfTheRequester) {
    Modules modules;
    modules.sources["a"] = "return 1";
    modules.sources["b"] = "return require('a')";

    modules.require("b");

    EXPECT_EQ(modules.requesters, (Keys{"", "b"}));
}

TEST(ModuleLoader, ARuntimeRequireOfALoadedModuleWorks) {
    Modules modules;
    modules.sources["dep"] = "return 'dep'";
    modules.sources["m"] = "require('dep'); return function() return require('dep') end";

    const Ref function = modules.require("m");

    EXPECT_EQ(modules.call(function), "dep");
}

TEST(ModuleLoader, ARuntimeRequireOfAnUnloadedModuleFails) {
    Modules modules;
    modules.sources["dep"] = "return 'dep'";
    modules.sources["m"] = "return function() return require('dep') end";

    const Ref function = modules.require("m");

    EXPECT_EQ(modules.call(function),
              "error: m.luau:1: require(\"dep\"): 'dep.luau' is not loaded: require it at the top of a module");
    EXPECT_FALSE(modules.loader().isLoaded("dep"));
}

TEST(ModuleLoader, ARuntimeRequireMayLoadDuringATransaction) {
    Modules modules;
    modules.sources["dep"] = "return 'dep'";
    modules.sources["m"] = "return function() return require('dep') end";
    const Ref function = modules.require("m");

    modules.loader().begin();
    EXPECT_EQ(modules.call(function), "dep");
    modules.loader().commit();

    EXPECT_TRUE(modules.loader().isLoaded("dep"));
}

TEST(ModuleLoader, TheRunHookIsNestedAroundEachRun) {
    Modules modules;
    modules.sources["a"] = "require('b'); require('c')";
    modules.sources["b"] = "require('c')";
    modules.sources["c"] = "return 1";

    modules.require("a");

    EXPECT_EQ(modules.events, (Keys{"+a", "+b", "+c", "-c", "-b", "-a"}));
}

TEST(ModuleLoader, TheRunHookIsCalledOnFailureToo) {
    Modules modules;
    modules.sources["a"] = "require('b')";
    modules.sources["b"] = "error('no')";
    modules.sources["c"] = "return 1, 2";

    EXPECT_FALSE(modules.loader().require("a"));
    EXPECT_FALSE(modules.loader().require("c"));

    EXPECT_EQ(modules.events, (Keys{"+a", "+b", "-b", "-a", "+c", "-c"}));
}

/// @brief a requires b and c; b requires d; c requires d; e requires a; f stands alone.
void fillDiamond(Modules& modules) {
    modules.sources["d"] = "return 1";
    modules.sources["b"] = "return require('d')";
    modules.sources["c"] = "return require('d')";
    modules.sources["a"] = "require('c'); require('b')";
    modules.sources["e"] = "require('a')";
    modules.sources["f"] = "return 1";
}

TEST(ModuleLoader, DependentsAreTransitiveAndTopological) {
    Modules modules;
    fillDiamond(modules);
    modules.require("e");
    modules.require("f");

    EXPECT_EQ(modules.loader().dependents({"d"}), (Keys{"d", "b", "c", "a", "e"}));
    EXPECT_EQ(modules.loader().dependents({"b"}), (Keys{"b", "a", "e"}));
    EXPECT_EQ(modules.loader().dependents({"e"}), (Keys{"e"}));
    EXPECT_EQ(modules.loader().dependents({"b", "c"}), (Keys{"b", "c", "a", "e"}));
}

TEST(ModuleLoader, ACacheHitStillRecordsTheEdge) {
    Modules modules;
    modules.sources["d"] = "return 1";
    modules.sources["x"] = "return require('d')";
    modules.require("d");
    modules.require("x");

    EXPECT_EQ(modules.loader().dependents({"d"}), (Keys{"d", "x"}));
}

TEST(ModuleLoader, ReachableFromFollowsTheRequires) {
    Modules modules;
    fillDiamond(modules);
    modules.require("e");
    modules.require("f");

    EXPECT_EQ(modules.loader().reachableFrom({"a"}), (Keys{"a", "b", "c", "d"}));
    EXPECT_EQ(modules.loader().reachableFrom({"e", "f"}), (Keys{"a", "b", "c", "d", "e", "f"}));
    EXPECT_EQ(modules.loader().reachableFrom({"d"}), (Keys{"d"}));
}

TEST(ModuleLoader, InvalidateThenCommitReloadsAndDropsTheOldEntries) {
    Modules modules;
    modules.sources["a"] = "return 1";
    modules.sources["b"] = "return require('a') + 1";
    modules.require("b");
    modules.events.clear();

    modules.sources["a"] = "return 10";
    modules.loader().begin();
    modules.loader().invalidate(modules.loader().dependents({"a"}));
    EXPECT_FALSE(modules.loader().isLoaded("a"));
    EXPECT_EQ(modules.text(modules.require("b")), "11");
    modules.loader().commit();

    EXPECT_FALSE(modules.loader().isInTransaction());
    EXPECT_EQ(modules.events, (Keys{"+b", "+a", "-a", "-b"}));
    EXPECT_EQ(modules.text(*modules.loader().value("b")), "11");
}

TEST(ModuleLoader, RollbackRestoresTheCacheAndTheGraph) {
    Modules modules;
    modules.sources["a"] = "return 1";
    modules.sources["b"] = "return require('a') + 1";
    modules.require("b");
    const Keys before = modules.loader().loadedKeys();

    modules.sources["a"] = "return 10";
    modules.sources["b"] = "return require('new') + 1";
    modules.sources["new"] = "return 5";
    modules.loader().begin();
    modules.loader().invalidate({"a", "b"});
    EXPECT_EQ(modules.text(modules.require("b")), "6");
    EXPECT_TRUE(modules.loader().isLoaded("new"));
    modules.loader().rollback();

    EXPECT_EQ(modules.loader().loadedKeys(), before);
    EXPECT_EQ(modules.text(*modules.loader().value("b")), "2");
    EXPECT_EQ(modules.text(*modules.loader().value("a")), "1");
    EXPECT_EQ(modules.loader().reachableFrom({"b"}), (Keys{"a", "b"}));
    EXPECT_EQ(modules.loader().dependents({"a"}), (Keys{"a", "b"}));
}

TEST(ModuleLoader, RollbackAfterAFailedReloadRestoresTheOldModules) {
    Modules modules;
    modules.sources["a"] = "return 1";
    modules.require("a");

    modules.sources["a"] = "error('broken')";
    modules.loader().begin();
    modules.loader().invalidate({"a"});
    EXPECT_FALSE(modules.loader().require("a"));
    modules.loader().rollback();

    EXPECT_EQ(modules.text(*modules.loader().value("a")), "1");
}

TEST(ModuleLoader, UnloadRemovesModulesAndTheirEdges) {
    Modules modules;
    fillDiamond(modules);
    modules.require("a");

    modules.loader().unload({"b", "nothing"});

    EXPECT_FALSE(modules.loader().isLoaded("b"));
    EXPECT_TRUE(modules.loader().isLoaded("c"));
    EXPECT_EQ(modules.loader().reachableFrom({"b"}), (Keys{"b"}));
}

TEST(ModuleLoader, UnloadInATransactionCanBeRolledBack) {
    Modules modules;
    modules.sources["a"] = "return 1";
    modules.require("a");

    modules.loader().begin();
    modules.loader().unload({"a"});
    EXPECT_FALSE(modules.loader().isLoaded("a"));
    modules.loader().rollback();

    EXPECT_TRUE(modules.loader().isLoaded("a"));
}

TEST(ModuleLoader, TransactionMisuseThrows) {
    Modules modules;

    EXPECT_THROW(modules.loader().invalidate({"a"}), rtype::luau::exceptions::InvalidOperation);
    EXPECT_THROW(modules.loader().commit(), rtype::luau::exceptions::InvalidOperation);
    EXPECT_THROW(modules.loader().rollback(), rtype::luau::exceptions::InvalidOperation);
    modules.loader().begin();
    EXPECT_THROW(modules.loader().begin(), rtype::luau::exceptions::InvalidOperation);
    modules.loader().commit();
}

TEST(ModuleLoader, ARequireAfterTheLoaderIsGoneRaisesAnError) {
    Modules modules;
    modules.sources["m"] = "return function() return require('m') end";
    const Ref function = modules.require("m");

    modules.dropLoader();

    EXPECT_NE(modules.call(function).find("the module loader no longer exists"), std::string::npos);
}
}  // namespace
