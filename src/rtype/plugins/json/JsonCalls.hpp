/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JsonCalls
*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/json/Export.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/call/CallTraits.hpp"

namespace rtype::plugins::json {

inline constexpr std::string_view kEncodeCall = "rtype.json.encode";  ///< detail::encode().
inline constexpr std::string_view kDecodeCall = "rtype.json.decode";  ///< detail::decode().

/// @brief The kind of one node of a value flattened into a pre-order stream, the way `EncodeArguments` and
/// `DecodeResult` carry it (docs/Json.md): the real encoding and decoding (number formatting, string escaping, JSON
/// grammar) is nlohmann::json's, in C++; `JsonForms.luau` only walks a Luau table's shape (array, object, or an
/// `engine.json.map()` ordered object) and rebuilds a Luau value from the stream, with no JSON logic of its own.
///
/// @details Reading the stream is recursive: a `kNumber` node consumes the next value of `numbers`; a `kString` node,
/// or one pair's key of a `kObject`/`kOrderedObject` node, consumes the next length of the lengths list and that many
/// bytes of the text pool, in the order the nodes need them; a `kArray` node consumes the next value of `counts`, its
/// element count, that many child nodes following it; a `kObject`/`kOrderedObject` node also consumes the next value
/// of `counts`, its pair count, that many (key, child node) pairs following it. `kNull`, `kFalse` and `kTrue` nodes
/// carry no data of their own.
enum class NodeKind : std::uint8_t {
    kNull,
    kFalse,
    kTrue,
    kNumber,
    kString,
    kArray,
    kObject,         ///< Written with its keys sorted (decode never gives this order any meaning).
    kOrderedObject,  ///< Encode only (`engine.json.map()`): written with its keys kept in the given order.
};

/// @brief The arguments of `rtype.json.encode`: a value flattened into a pre-order node stream by `JsonForms.luau`.
struct EncodeArguments {
    std::vector<std::uint8_t> kinds;         ///< Each node's NodeKind, in pre-order.
    std::vector<double> numbers;             ///< One value per `kNumber` node, in order.
    std::vector<std::uint32_t> counts;       ///< One value per `kArray`/`kObject`/`kOrderedObject` node, in order.
    std::string text;                        ///< Every string node's bytes and every key's bytes, concatenated.
    std::vector<std::uint32_t> textLengths;  ///< The length, in `text`, of each piece above, in the order it is read.
};
RTYPE_ECS_CALL_DATA(EncodeArguments, "rtype.json.EncodeArguments", kinds, numbers, counts, text, textLengths);

/// @brief The result of `rtype.json.encode`.
struct EncodeResult {
    std::string text;  ///< The JSON text.
};
RTYPE_ECS_CALL_DATA(EncodeResult, "rtype.json.EncodeResult", text);

/// @brief The arguments of `rtype.json.decode`.
struct DecodeArguments {
    std::string text;  ///< The JSON text to parse.
};
RTYPE_ECS_CALL_DATA(DecodeArguments, "rtype.json.DecodeArguments", text);

/// @brief The result of `rtype.json.decode`: the parsed value, flattened the same way as `EncodeArguments` (never a
/// `kOrderedObject`: a decoded object has no order of its own for `JsonForms.luau` to keep). `JsonForms.luau` rebuilds
/// a Luau value from it.
struct DecodeResult {
    std::vector<std::uint8_t> kinds;
    std::vector<double> numbers;
    std::vector<std::uint32_t> counts;
    std::string strings;
    std::vector<std::uint32_t> stringLengths;
};
RTYPE_ECS_CALL_DATA(DecodeResult, "rtype.json.DecodeResult", kinds, numbers, counts, strings, stringLengths);

/// @brief Registers `rtype.json.encode` and `rtype.json.decode` on @p app, and the forms (addJsonForms()). Called by
/// the JsonPlugin.
RTYPE_PLUGIN_JSON_API void addJsonCalls(ecs::App& app);

/// @brief Registers the Luau helpers of `engine.json` on @p registry (`JsonForms.luau`): `encode`, `decode`,
/// `tryDecode`, `map`. Its library does it on every side, and so does building the plugin, for a program without
/// libraries.
RTYPE_PLUGIN_JSON_API void addJsonForms(ecs::PluginRegistry& registry);

}  // namespace rtype::plugins::json
