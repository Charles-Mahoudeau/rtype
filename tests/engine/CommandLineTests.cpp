/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandLineTests
*/

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "rtype/engine/CommandLine.hpp"
#include "rtype/engine/Exceptions.hpp"

namespace {
using rtype::engine::CommandLine;
using rtype::engine::exceptions::RuntimeError;

/// @return The flags of @p args.
CommandLine parse(const std::vector<std::string>& args) { return CommandLine::parse(args); }
}  // namespace

TEST(CommandLine, ParsesNothingFromNoArguments) { EXPECT_TRUE(parse({}).flags().empty()); }

TEST(CommandLine, ReadsAFlagAndTheValueAfterIt) {
    const CommandLine commandLine = parse({"--game", "levels/one", "--port", "4243"});

    EXPECT_EQ(commandLine.get("game"), "levels/one");
    EXPECT_EQ(commandLine.get("port"), "4243");
    EXPECT_EQ(commandLine.flags().size(), 2U);
}

TEST(CommandLine, ReadsAValueAfterAnEqualSign) {
    const CommandLine commandLine = parse({"--host=10.0.0.2", "--empty="});

    EXPECT_EQ(commandLine.get("host"), "10.0.0.2");
    EXPECT_EQ(commandLine.get("empty"), "");
}

TEST(CommandLine, GivesTrueToABareFlag) {
    const CommandLine commandLine = parse({"--verbose", "--port", "1", "--fullscreen"});

    EXPECT_EQ(commandLine.get("verbose"), "true");     // followed by another flag
    EXPECT_EQ(commandLine.get("fullscreen"), "true");  // last
}

TEST(CommandLine, TakesANegativeNumberAsAValue) { EXPECT_EQ(parse({"--offset", "-3"}).get("offset"), "-3"); }

TEST(CommandLine, KeepsTheLastValueOfAFlagGivenTwice) {
    EXPECT_EQ(parse({"--port", "1", "--port", "2"}).get("port"), "2");
}

TEST(CommandLine, GivesNothingForAMissingFlag) { EXPECT_FALSE(parse({"--port", "1"}).get("host")); }

TEST(CommandLine, RejectsAnArgumentThatIsNotAFlag) {
    EXPECT_THROW(static_cast<void>(parse({"server"})), RuntimeError);
    EXPECT_THROW(static_cast<void>(parse({"--port", "1", "2"})), RuntimeError);
    EXPECT_THROW(static_cast<void>(parse({"-p", "1"})), RuntimeError);
}

TEST(CommandLine, RejectsAFlagWithoutAName) {
    EXPECT_THROW(static_cast<void>(parse({"--"})), RuntimeError);
    EXPECT_THROW(static_cast<void>(parse({"--=1"})), RuntimeError);
}

TEST(CommandLine, SetsAFlag) {
    CommandLine commandLine = parse({"--port", "1"});

    commandLine.set("port", "2");
    commandLine.set("host", "localhost");
    EXPECT_EQ(commandLine.get("port"), "2");
    EXPECT_EQ(commandLine.get("host"), "localhost");
}

TEST(CommandLine, TurnsBooleansIntoBooleans) {
    EXPECT_EQ(CommandLine::toValue("true"), CommandLine::Value{true});
    EXPECT_EQ(CommandLine::toValue("false"), CommandLine::Value{false});
    EXPECT_EQ(CommandLine::toValue("True"), CommandLine::Value{std::string{"True"}});
}

TEST(CommandLine, TurnsDecimalNumbersIntoNumbers) {
    EXPECT_EQ(CommandLine::toValue("4242"), CommandLine::Value{4242.0});
    EXPECT_EQ(CommandLine::toValue("-3"), CommandLine::Value{-3.0});
    EXPECT_EQ(CommandLine::toValue("+0.5"), CommandLine::Value{0.5});
    EXPECT_EQ(CommandLine::toValue(".5"), CommandLine::Value{0.5});
    EXPECT_EQ(CommandLine::toValue("2."), CommandLine::Value{2.0});
    EXPECT_EQ(CommandLine::toValue("1e3"), CommandLine::Value{1000.0});
    EXPECT_EQ(CommandLine::toValue("1.5E-1"), CommandLine::Value{0.15});
}

TEST(CommandLine, KeepsEverythingElseAsAString) {
    for (const std::string text :
         {"", "localhost", "10.0.0.2", "1e", "-", ".", "+", "inf", "nan", "0x10", "12abc", " 1", "1 "}) {
        EXPECT_EQ(CommandLine::toValue(text), CommandLine::Value{text}) << text;
    }
}
