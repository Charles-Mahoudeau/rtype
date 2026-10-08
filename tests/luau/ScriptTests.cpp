/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptTests
*/

#include <gtest/gtest.h>
#include <lua.h>
#include <lualib.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "rtype/luau/Ref.hpp"
#include "rtype/luau/Script.hpp"

namespace {
struct StateCloser {
    void operator()(lua_State* state) const noexcept { lua_close(state); }
};

using State = std::unique_ptr<lua_State, StateCloser>;

rtype::luau::Script makeScript(lua_State& state, std::string name) {
    lua_State* thread = lua_newthread(&state);
    rtype::luau::Ref threadRef = rtype::luau::Ref::pop(&state);
    lua_pushinteger(thread, 5);
    rtype::luau::Ref closure = rtype::luau::Ref::pop(thread);
    return rtype::luau::Script{thread, std::move(threadRef), std::move(closure), std::move(name)};
}
}  // namespace

TEST(Script, ExposesName) {
    const State state{luaL_newstate()};

    const auto script = makeScript(*state, "main.luau");

    EXPECT_EQ(script.name(), "main.luau");
}

TEST(Script, ExposesThread) {
    const State state{luaL_newstate()};

    const auto script = makeScript(*state, "main.luau");

    EXPECT_NE(script.state(), nullptr);
    EXPECT_NE(script.state(), state.get());
    script.thread().push();
    EXPECT_EQ(lua_tothread(state.get(), -1), script.state());
}

TEST(Script, ExposesClosure) {
    const State state{luaL_newstate()};
    const auto script = makeScript(*state, "main.luau");

    script.closure().push();

    EXPECT_EQ(lua_tointeger(state.get(), -1), 5);
}

TEST(Script, MoveConstructionTransfersParts) {
    const State state{luaL_newstate()};
    auto source = makeScript(*state, "moved.luau");

    const rtype::luau::Script moved{std::move(source)};

    EXPECT_EQ(moved.name(), "moved.luau");
    EXPECT_NE(moved.state(), nullptr);
}

TEST(Script, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<rtype::luau::Script>);
    static_assert(!std::is_copy_assignable_v<rtype::luau::Script>);
    static_assert(std::is_nothrow_move_constructible_v<rtype::luau::Script>);
    static_assert(std::is_nothrow_move_assignable_v<rtype::luau::Script>);
    SUCCEED();
}
