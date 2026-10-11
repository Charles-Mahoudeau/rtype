/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallContextTests
*/

#include <gtest/gtest.h>

#include <string>
#include <utility>

#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace {
using CallContext = rtype::luau::CallContext;
using ValuePack = rtype::luau::ValuePack;

}  // namespace

TEST(CallContext, IsEmptyByDefault) {
    const CallContext context{};

    EXPECT_EQ(context.argCount(), 0U);
    EXPECT_TRUE(context.args().empty());
}

TEST(CallContext, KeepsArgumentsInOrder) {
    ValuePack args{};
    args.add({1.0});
    args.add({std::string{"two"}});

    const CallContext context{std::move(args)};

    ASSERT_EQ(context.argCount(), 2U);
    EXPECT_EQ(context.args()[0].as<double>(), 1.0);
    EXPECT_EQ(context.args()[1].as<std::string>(), "two");
}

TEST(CallContext, MoveKeepsArguments) {
    ValuePack args{};
    args.add({true});
    CallContext context{std::move(args)};

    const CallContext moved{std::move(context)};

    EXPECT_EQ(moved.argCount(), 1U);
}
