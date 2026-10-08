/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RefTests
*/

#include <gtest/gtest.h>
#include <lua.h>
#include <lualib.h>

#include <memory>
#include <type_traits>
#include <utility>

#include "rtype/luau/Ref.hpp"

namespace {
struct StateCloser {
    void operator()(lua_State* state) const noexcept { lua_close(state); }
};

using State = std::unique_ptr<lua_State, StateCloser>;

State makeState() {
    State state{luaL_newstate()};
    luaL_openlibs(state.get());
    return state;
}
}  // namespace

TEST(Ref, PopsValueByDefault) {
    const State state = makeState();
    lua_pushinteger(state.get(), 42);

    const rtype::luau::Ref ref{*state};

    EXPECT_EQ(lua_gettop(state.get()), 0);
}

TEST(Ref, KeepsValueOnStackWhenRequested) {
    const State state = makeState();
    lua_pushinteger(state.get(), 42);

    const rtype::luau::Ref ref{*state, -1, true};

    EXPECT_EQ(lua_gettop(state.get()), 1);
}

TEST(Ref, PushRestoresReferencedValue) {
    const State state = makeState();
    lua_pushinteger(state.get(), 42);
    const rtype::luau::Ref ref{*state};

    ref.push();

    ASSERT_EQ(lua_gettop(state.get()), 1);
    EXPECT_EQ(lua_tointeger(state.get(), -1), 42);
}

TEST(Ref, PushCanBeRepeated) {
    const State state = makeState();
    lua_pushstring(state.get(), "hello");
    const rtype::luau::Ref ref{*state};

    ref.push();
    ref.push();

    EXPECT_EQ(lua_gettop(state.get()), 2);
    EXPECT_STREQ(lua_tostring(state.get(), -1), "hello");
    EXPECT_STREQ(lua_tostring(state.get(), -2), "hello");
}

TEST(Ref, ReferencesValueAtGivenIndex) {
    const State state = makeState();
    lua_pushinteger(state.get(), 1);
    lua_pushinteger(state.get(), 2);

    const rtype::luau::Ref ref{*state, 1, true};
    ref.push();

    EXPECT_EQ(lua_tointeger(state.get(), -1), 1);
}

TEST(Ref, MoveConstructionTransfersReference) {
    const State state = makeState();
    lua_pushinteger(state.get(), 7);
    rtype::luau::Ref source{*state};

    const rtype::luau::Ref moved{std::move(source)};
    moved.push();

    EXPECT_EQ(lua_tointeger(state.get(), -1), 7);
}

TEST(Ref, MoveAssignmentTransfersReference) {
    const State state = makeState();
    lua_pushinteger(state.get(), 1);
    rtype::luau::Ref target{*state};
    lua_pushinteger(state.get(), 2);
    rtype::luau::Ref source{*state};

    target = std::move(source);
    target.push();

    EXPECT_EQ(lua_tointeger(state.get(), -1), 2);
}

TEST(Ref, SelfMoveAssignmentKeepsReference) {
    const State state = makeState();
    lua_pushinteger(state.get(), 9);
    rtype::luau::Ref ref{*state};
    rtype::luau::Ref& alias = ref;

    ref = std::move(alias);
    ref.push();

    EXPECT_EQ(lua_tointeger(state.get(), -1), 9);
}

TEST(Ref, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<rtype::luau::Ref>);
    static_assert(!std::is_copy_assignable_v<rtype::luau::Ref>);
    static_assert(std::is_nothrow_move_constructible_v<rtype::luau::Ref>);
    static_assert(std::is_nothrow_move_assignable_v<rtype::luau::Ref>);
    SUCCEED();
}
