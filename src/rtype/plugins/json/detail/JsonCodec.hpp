/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JsonCodec
*/

#pragma once

#include "plugins/json/JsonCalls.hpp"

namespace rtype::plugins::json::detail {

/// @brief Builds the JSON text of the flattened node stream of @p arguments (JsonCalls.hpp), writing every number and
/// string with nlohmann::json. A `kObject` node is written with its keys sorted; a `kOrderedObject` node (Luau's
/// `json.map()`) keeps the order given.
/// @throws exceptions::JsonException Naming the issue: a NaN or infinite number, or a malformed stream (fewer nodes,
/// numbers, counts or text than the stream calls for, or nesting over 64 levels).
[[nodiscard]] EncodeResult encode(const EncodeArguments& arguments);

/// @brief Parses @p arguments.text as JSON with nlohmann::json and flattens the result into the node stream
/// `DecodeResult` describes (JsonCalls.hpp), which `JsonForms.luau` rebuilds into a Luau value.
/// @throws exceptions::JsonException Naming the byte of the error: malformed text, nesting over 64 levels, or a
/// number too large for a finite double.
[[nodiscard]] DecodeResult decode(const DecodeArguments& arguments);

}  // namespace rtype::plugins::json::detail
