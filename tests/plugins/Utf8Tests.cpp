/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Utf8Tests
*/

#include <gtest/gtest.h>

#include <string>

#include "plugins/text/detail/Utf8.hpp"

using rtype::plugins::text::detail::decodeUtf8;
using rtype::plugins::text::detail::kReplacementCharacter;

TEST(Utf8, DecodesAscii) { EXPECT_EQ(decodeUtf8("R-Type 1"), U"R-Type 1"); }

TEST(Utf8, DecodesEverySequenceLength) {
    EXPECT_EQ(decodeUtf8("\xC3\xA9"), U"\u00E9");              // An e with an acute accent: 2 bytes.
    EXPECT_EQ(decodeUtf8("\xE2\x82\xAC"), U"\u20AC");          // The euro sign: 3 bytes.
    EXPECT_EQ(decodeUtf8("\xF0\x9F\x9A\x80"), U"\U0001F680");  // A rocket: 4 bytes.
    EXPECT_EQ(decodeUtf8("a\xC3\xA9z"), U"a\u00E9z");
}

TEST(Utf8, ReplacesEachInvalidByte) {
    const std::u32string replaced(1, kReplacementCharacter);
    EXPECT_EQ(decodeUtf8("\x80"), replaced);                                // A stray continuation byte.
    EXPECT_EQ(decodeUtf8("\xC3"), replaced);                                // A truncated sequence.
    EXPECT_EQ(decodeUtf8("\xC0\xAF"), replaced + replaced);                 // An overlong '/'.
    EXPECT_EQ(decodeUtf8("\xED\xA0\x80"), replaced + replaced + replaced);  // A surrogate.
    EXPECT_EQ(decodeUtf8("a\xFFz"), U"a" + replaced + U"z");
}

TEST(Utf8, DecodesNothingFromNothing) { EXPECT_TRUE(decodeUtf8("").empty()); }
