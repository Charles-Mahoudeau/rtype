/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptJsonTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

#include "plugins/json/JsonPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
/// @brief A standalone App with the JsonPlugin, and a script host on it.
struct JsonScripts {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    JsonScripts() {
        app.addPlugin(rtype::plugins::json::JsonPlugin{});
        auto created = rtype::scripting::ScriptHost::create(app, {.isClient = true, .isServer = true});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("json-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptJson, EncodesWithSortedKeysAndArrays) {
    JsonScripts s;
    EXPECT_EQ(s.run(R"(
        local json = engine.json
        assert(json.encode({ b = 1, a = { 1, 2, 3 }, c = "x\n\"y\"" }) == '{"a":[1,2,3],"b":1,"c":"x\\n\\"y\\""}')
        assert(json.encode(nil) == "null")
        assert(json.encode({}) == "[]")
        assert(json.encode(true) == "true")
        assert(json.encode(1.5) == "1.5")
        assert(json.encode(-3) == "-3")
        assert(json.encode("\1") == '"\\u0001"')
        assert(json.encode({ 1, { x = false } }) == '[1,{"x":false}]')
    )"),
              "");
}

TEST(ScriptJson, AnOrderedMapKeepsItsKeyOrder) {
    JsonScripts s;
    EXPECT_EQ(s.run(R"(
        local json = engine.json
        local text = json.encode(json.map({ { "z", 1 }, { "a", json.map({ { "y", 2 }, { "b", { 3 } } }) } }))
        assert(text == '{"z":1,"a":{"y":2,"b":[3]}}', text)
        assert(json.encode({ inner = json.map({ { "b", 1 }, { "a", 2 } }) }) == '{"inner":{"b":1,"a":2}}')
        assert(not pcall(json.map, 3))
        assert(not pcall(json.encode, json.map({ { 1, 2 } })))
    )"),
              "");
}

TEST(ScriptJson, EncodeRefusesWhatJsonCannotHold) {
    JsonScripts s;
    EXPECT_EQ(s.run(R"(
        local json = engine.json
        local function fails(value, part)
            local ok, message = pcall(json.encode, value)
            assert(not ok, "expected an error")
            assert(string.find(message, part, 1, true), message)
        end
        fails(0 / 0, "NaN")
        fails(math.huge, "inf")
        fails(function() end, "function")
        fails({ 1, 2, a = 3 }, "mix")
        fails({ [1] = 1, [3] = 3 }, "keys")
        local loop = {}
        loop.self = loop
        fails(loop, "nested too deeply")
    )"),
              "");
}

TEST(ScriptJson, DecodesEveryKindOfValue) {
    JsonScripts s;
    EXPECT_EQ(s.run(R"(
        local json = engine.json
        local v = json.decode(' { "a" : [1, 2.5, -3e2, true, false, "s"], "o": {"k": "v"}, "n": null, "e": [] } ')
        assert(v.a[1] == 1 and v.a[2] == 2.5 and v.a[3] == -300 and v.a[4] == true and v.a[5] == false and v.a[6] == "s")
        assert(v.o.k == "v" and v.n == nil and #v.e == 0)
        assert(json.decode("null") == nil)
        assert(json.decode("7") == 7)
        assert(json.decode('"\\u00e9\\ud83d\\ude00\\n\\/"') == "\195\169\240\159\152\128\n/")
        assert(not json.tryDecode('"\\ud800x"')) -- a lone surrogate is not text
        assert(json.decode('"\127"') == "\127")
    )"),
              "");
}

TEST(ScriptJson, RoundTripsADocument) {
    JsonScripts s;
    EXPECT_EQ(s.run(R"(
        local json = engine.json
        local value = { name = "ship", pos = { x = 1.25, y = -2 }, tags = { "a", "b" }, alive = true, text = "tab\tq\"" }
        local text = json.encode(value)
        assert(json.encode(json.decode(text)) == text)
        assert(json.decode(text).pos.x == 1.25)
        assert(json.decode(json.encode(0.1)) == 0.1)
    )"),
              "");
}

TEST(ScriptJson, DecodeErrorsNameTheByte) {
    JsonScripts s;
    EXPECT_EQ(s.run(R"(
        local json = engine.json
        local function fails(text, part)
            local ok, message = pcall(json.decode, text)
            assert(not ok, "expected an error for " .. text)
            assert(string.find(message, part, 1, true), message)
        end
        fails("", "malformed JSON at byte 1")
        fails("{", "malformed JSON at byte")
        fails('{"a" 1}', "malformed JSON at byte 6")
        fails("[1 2]", "malformed JSON at byte 4")
        fails("[1,]", "malformed JSON at byte 4")
        fails('"abc', "malformed JSON")
        fails('"a\nb"', "malformed JSON")
        fails('"\\q"', "malformed JSON")
        fails('"\\u12"', "malformed JSON")
        fails("1 2", "malformed JSON at byte 3")
        fails("01", "malformed JSON")
        fails("1e999", "out of range")
        fails("nul", "malformed JSON")
        fails("-", "malformed JSON")
        fails(string.rep("[", 65) .. string.rep("]", 65), "nested too deeply")
        assert(not pcall(json.decode, 5))
        json.decode(string.rep("[", 64) .. string.rep("]", 64))
    )"),
              "");
}

TEST(ScriptJson, TryDecodeNeverThrows) {
    JsonScripts s;
    EXPECT_EQ(s.run(R"(
        local json = engine.json
        local value, err = json.tryDecode('{"a":1}')
        assert(value.a == 1 and err == nil)
        value, err = json.tryDecode("{oops")
        assert(value == nil and string.find(err, "byte", 1, true), err)
        value, err = json.tryDecode(nil)
        assert(value == nil and err ~= nil)
        value, err = json.tryDecode("null")
        assert(value == nil and err == nil)
    )"),
              "");
}

TEST(ScriptJson, TheNativeCallBoundsNestingEvenWhenCalledDirectly) {
    JsonScripts s;
    EXPECT_EQ(s.run(R"(
        local kinds, counts = {}, {}
        for i = 1, 70 do kinds[i] = 5; counts[i] = 1 end -- 70 arrays, each holding the next
        kinds[71] = 0
        local ok, message = pcall(engine.call, "rtype.json.encode", { kinds = kinds, counts = counts })
        assert(not ok and string.find(message, "nested too deeply", 1, true), message)
    )"),
              "");
}
