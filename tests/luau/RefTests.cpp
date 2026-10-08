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
#include <stdexcept>
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

TEST(Ref, PopRemovesValueFromStack) {
    const State state = makeState();
    lua_pushinteger(state.get(), 42);

    const auto ref = rtype::luau::Ref::pop(state.get());

    EXPECT_EQ(lua_gettop(state.get()), 0);
}

TEST(Ref, ConstructorKeepsValueOnStack) {
    const State state = makeState();
    lua_pushinteger(state.get(), 42);

    const rtype::luau::Ref ref{state.get()};

    EXPECT_EQ(lua_gettop(state.get()), 1);
}

TEST(Ref, ThrowsOnNullState) { EXPECT_THROW((rtype::luau::Ref{nullptr}), std::runtime_error); }

TEST(Ref, PushOntoAnotherThread) {
    const State state = makeState();
    lua_State* thread = lua_newthread(state.get());
    const auto threadRef = rtype::luau::Ref::pop(state.get());
    lua_pushinteger(state.get(), 11);
    const auto ref = rtype::luau::Ref::pop(state.get());

    ref.push(thread);

    EXPECT_EQ(lua_tointeger(thread, -1), 11);
}

TEST(Ref, ReferenceCreatedFromThreadIsUsableFromMainState) {
    const State state = makeState();
    lua_State* thread = lua_newthread(state.get());
    const auto threadRef = rtype::luau::Ref::pop(state.get());
    lua_pushinteger(thread, 21);
    const auto ref = rtype::luau::Ref::pop(thread);

    ref.push(state.get());

    EXPECT_EQ(lua_tointeger(state.get(), -1), 21);
}

TEST(Ref, PushWithNullStateUsesOriginState) {
    const State state = makeState();
    lua_pushinteger(state.get(), 5);
    const auto ref = rtype::luau::Ref::pop(state.get());

    ref.push(nullptr);

    EXPECT_EQ(lua_tointeger(state.get(), -1), 5);
}

TEST(Ref, MovedFromRefCanBeAssignedAgain) {
    const State state = makeState();
    lua_pushinteger(state.get(), 1);
    auto source = rtype::luau::Ref::pop(state.get());
    const rtype::luau::Ref moved{std::move(source)};
    lua_pushinteger(state.get(), 2);

    source = rtype::luau::Ref::pop(state.get());
    source.push();

    EXPECT_EQ(lua_tointeger(state.get(), -1), 2);
}

TEST(Ref, PushRestoresReferencedValue) {
    const State state = makeState();
    lua_pushinteger(state.get(), 42);
    const auto ref = rtype::luau::Ref::pop(state.get());

    ref.push();

    ASSERT_EQ(lua_gettop(state.get()), 1);
    EXPECT_EQ(lua_tointeger(state.get(), -1), 42);
}

TEST(Ref, PushCanBeRepeated) {
    const State state = makeState();
    lua_pushstring(state.get(), "hello");
    const auto ref = rtype::luau::Ref::pop(state.get());

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

    const rtype::luau::Ref ref{state.get(), 1};
    ref.push();

    EXPECT_EQ(lua_tointeger(state.get(), -1), 1);
}

TEST(Ref, MoveConstructionTransfersReference) {
    const State state = makeState();
    lua_pushinteger(state.get(), 7);
    auto source = rtype::luau::Ref::pop(state.get());

    const rtype::luau::Ref moved{std::move(source)};
    moved.push();

    EXPECT_EQ(lua_tointeger(state.get(), -1), 7);
}

TEST(Ref, MoveAssignmentTransfersReference) {
    const State state = makeState();
    lua_pushinteger(state.get(), 1);
    auto target = rtype::luau::Ref::pop(state.get());
    lua_pushinteger(state.get(), 2);
    auto source = rtype::luau::Ref::pop(state.get());

    target = std::move(source);
    target.push();

    EXPECT_EQ(lua_tointeger(state.get(), -1), 2);
}

TEST(Ref, SelfMoveAssignmentKeepsReference) {
    const State state = makeState();
    lua_pushinteger(state.get(), 9);
    auto ref = rtype::luau::Ref::pop(state.get());
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
