/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptTests
*/

#include <gtest/gtest.h>
#include <lua.h>
#include <lualib.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "rtype/luau/Bytecode.hpp"
#include "rtype/luau/CHelper.hpp"
#include "rtype/luau/Ref.hpp"
#include "rtype/luau/Script.hpp"

namespace {
struct StateCloser {
    void operator()(lua_State* state) const noexcept { lua_close(state); }
};

using State = std::unique_ptr<lua_State, StateCloser>;

rtype::luau::Script makeScript(lua_State& state, std::string name) {
    lua_pushinteger(&state, 5);
    // NOLINTNEXTLINE(*-avoid-c-arrays,*-no-malloc,*-owning-memory)
    rtype::luau::Bytecode bytecode{rtype::luau::CPtr<char[]>{static_cast<char*>(std::malloc(3))}, 3};
    return rtype::luau::Script{rtype::luau::Ref{state}, std::move(name), std::move(bytecode)};
}
}  // namespace

TEST(Script, ExposesName) {
    const State state{luaL_newstate()};

    const auto script = makeScript(*state, "main.luau");

    EXPECT_EQ(script.name(), "main.luau");
}

TEST(Script, ExposesBytecode) {
    const State state{luaL_newstate()};

    const auto script = makeScript(*state, "main.luau");

    EXPECT_EQ(script.bytecode().size(), 3U);
    EXPECT_NE(script.bytecode().data().get(), nullptr);
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
    EXPECT_EQ(moved.bytecode().size(), 3U);
}

TEST(Script, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<rtype::luau::Script>);
    static_assert(!std::is_copy_assignable_v<rtype::luau::Script>);
    static_assert(std::is_nothrow_move_constructible_v<rtype::luau::Script>);
    static_assert(std::is_nothrow_move_assignable_v<rtype::luau::Script>);
    SUCCEED();
}
