/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** JsonCodec
*/

#include "plugins/json/detail/JsonCodec.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/json/JsonCalls.hpp"
#include "plugins/json/exceptions/JsonException.hpp"

namespace rtype::plugins::json::detail {
namespace {

using exceptions::JsonException;

constexpr std::uint32_t kMaxDepth = 64;
constexpr double kMaxSafeInteger = 9007199254740992.0;  // 2^53: the largest integer a double holds exactly.

/// @return @p text as a JSON string literal. A Luau string is bytes, not checked UTF-8 (docs/Json.md): an invalid
/// sequence is written as is, as the previous hand-written encoder did, instead of nlohmann::json's default of
/// throwing a `type_error`.
[[nodiscard]] std::string dumpString(const std::string& text) {
    return nlohmann::json(text).dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore);
}

/// @brief Reads the nodes of an EncodeArguments stream one at a time, building their JSON text.
class EncodeCursor {
  public:
    explicit EncodeCursor(const EncodeArguments& arguments) : _arguments{&arguments} {}

    /// @return The JSON text of the next node of the stream, and every node it contains.
    // NOLINTNEXTLINE(misc-no-recursion): the depth is bounded by checkDepth().
    [[nodiscard]] std::string next(const std::uint32_t depth) {
        if (_kindIndex >= _arguments->kinds.size()) {
            throw JsonException{"json: malformed encode arguments: fewer nodes than expected"};
        }
        switch (static_cast<NodeKind>(_arguments->kinds.at(_kindIndex++))) {
            case NodeKind::kNull:
                return "null";
            case NodeKind::kFalse:
                return "false";
            case NodeKind::kTrue:
                return "true";
            case NodeKind::kNumber:
                return number();
            case NodeKind::kString:
                return dumpString(nextText());
            case NodeKind::kArray:
                return array(depth);
            case NodeKind::kObject:
                return object(depth, true);
            case NodeKind::kOrderedObject:
                return object(depth, false);
        }
        throw JsonException{"json: malformed encode arguments: unknown node kind"};
    }

  private:
    [[nodiscard]] std::string number() {
        if (_numberIndex >= _arguments->numbers.size()) {
            throw JsonException{"json: malformed encode arguments: fewer numbers than expected"};
        }
        const double value = _arguments->numbers.at(_numberIndex++);
        if (std::isnan(value)) {
            throw JsonException{"json: cannot encode NaN"};
        }
        if (std::isinf(value)) {
            throw JsonException{std::format("json: cannot encode {}", value > 0 ? "inf" : "-inf")};
        }
        // A whole number under 2^53 is written without a fraction, as every integer up to it; a wider or fractional
        // one is nlohmann::json's own shortest round-tripping text.
        if (std::floor(value) == value && std::abs(value) < kMaxSafeInteger) {
            return nlohmann::json(static_cast<std::int64_t>(value)).dump();
        }
        return nlohmann::json(value).dump();
    }

    [[nodiscard]] std::string nextText() {
        if (_textLengthIndex >= _arguments->textLengths.size()) {
            throw JsonException{"json: malformed encode arguments: fewer strings than expected"};
        }
        const std::uint32_t length = _arguments->textLengths.at(_textLengthIndex++);
        if (_textOffset > _arguments->text.size() || length > _arguments->text.size() - _textOffset) {
            throw JsonException{"json: malformed encode arguments: the text pool is shorter than its lengths"};
        }
        std::string piece = _arguments->text.substr(_textOffset, length);
        _textOffset += length;
        return piece;
    }

    [[nodiscard]] std::uint32_t nextCount() {
        if (_countIndex >= _arguments->counts.size()) {
            throw JsonException{"json: malformed encode arguments: fewer counts than expected"};
        }
        return _arguments->counts.at(_countIndex++);
    }

    /// @brief Both the recursion guard against a node that is its own ancestor and the 64-level limit documented for
    /// scripts: this call is public API (a script may call it directly, skipping JsonForms.luau's own guard), so it
    /// must hold even then.
    static void checkDepth(const std::uint32_t depth) {
        if (depth >= kMaxDepth) {
            throw JsonException{
                "json: nested too deeply to encode (more than 64 levels, or a table that contains itself)"};
        }
    }

    // NOLINTNEXTLINE(misc-no-recursion): the depth is bounded by checkDepth().
    [[nodiscard]] std::string array(const std::uint32_t depth) {
        checkDepth(depth);
        const std::uint32_t count = nextCount();
        std::string text = "[";
        for (std::uint32_t i = 0; i < count; ++i) {
            if (i > 0) {
                text += ',';
            }
            text += next(depth + 1);
        }
        text += ']';
        return text;
    }

    // NOLINTNEXTLINE(misc-no-recursion): the depth is bounded by checkDepth().
    [[nodiscard]] std::string object(const std::uint32_t depth, const bool sorted) {
        checkDepth(depth);
        const std::uint32_t count = nextCount();
        std::vector<std::pair<std::string, std::string>> pairs;
        pairs.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            std::string key = nextText();
            std::string value = next(depth + 1);
            pairs.emplace_back(std::move(key), std::move(value));
        }
        if (sorted) {
            std::ranges::sort(pairs, {}, &std::pair<std::string, std::string>::first);
        }
        std::string text = "{";
        for (std::size_t i = 0; i < pairs.size(); ++i) {
            if (i > 0) {
                text += ',';
            }
            text += dumpString(pairs.at(i).first);
            text += ':';
            text += pairs.at(i).second;
        }
        text += '}';
        return text;
    }

    const EncodeArguments* _arguments;  ///< The stream read from.
    std::size_t _kindIndex{0};          ///< The next unread index of `_arguments->kinds`.
    std::size_t _numberIndex{0};        ///< The next unread index of `_arguments->numbers`.
    std::size_t _countIndex{0};         ///< The next unread index of `_arguments->counts`.
    std::size_t _textLengthIndex{0};    ///< The next unread index of `_arguments->textLengths`.
    std::size_t _textOffset{0};         ///< The next unread byte of `_arguments->text`.
};

/// @brief Appends @p text as the next piece of @p result's text pool.
void pushText(DecodeResult& result, const std::string_view text) {
    result.strings.append(text);
    result.stringLengths.push_back(static_cast<std::uint32_t>(text.size()));
}

/// @brief Flattens @p node, and every node it contains, into @p result (the reverse of EncodeCursor).
// NOLINTNEXTLINE(misc-no-recursion): the depth is bounded by kMaxDepth.
void flatten(const nlohmann::json& node, DecodeResult& result, const std::uint32_t depth) {
    switch (node.type()) {
        case nlohmann::json::value_t::null:
            result.kinds.push_back(static_cast<std::uint8_t>(NodeKind::kNull));
            return;
        case nlohmann::json::value_t::boolean:
            result.kinds.push_back(static_cast<std::uint8_t>(node.get<bool>() ? NodeKind::kTrue : NodeKind::kFalse));
            return;
        case nlohmann::json::value_t::number_integer:
        case nlohmann::json::value_t::number_unsigned:
        case nlohmann::json::value_t::number_float: {
            const double value = node.get<double>();
            if (std::isnan(value) || std::isinf(value)) {
                throw JsonException{"json: number out of range"};
            }
            result.kinds.push_back(static_cast<std::uint8_t>(NodeKind::kNumber));
            result.numbers.push_back(value);
            return;
        }
        case nlohmann::json::value_t::string:
            result.kinds.push_back(static_cast<std::uint8_t>(NodeKind::kString));
            pushText(result, node.get_ref<const std::string&>());
            return;
        case nlohmann::json::value_t::array:
            if (depth >= kMaxDepth) {
                throw JsonException{"json: nested too deeply to decode (more than 64 levels)"};
            }
            result.kinds.push_back(static_cast<std::uint8_t>(NodeKind::kArray));
            result.counts.push_back(static_cast<std::uint32_t>(node.size()));
            for (const auto& child : node) {
                flatten(child, result, depth + 1);
            }
            return;
        case nlohmann::json::value_t::object:
            if (depth >= kMaxDepth) {
                throw JsonException{"json: nested too deeply to decode (more than 64 levels)"};
            }
            result.kinds.push_back(static_cast<std::uint8_t>(NodeKind::kObject));
            result.counts.push_back(static_cast<std::uint32_t>(node.size()));
            for (const auto& [key, value] : node.items()) {
                pushText(result, key);
                flatten(value, result, depth + 1);
            }
            return;
        case nlohmann::json::value_t::discarded:
        case nlohmann::json::value_t::binary:
            break;
    }
    throw JsonException{"json: cannot decode this JSON value"};
}

/// @return @p text parsed as a JSON document, bounded to 64 levels of nesting.
/// @throws JsonException Naming the byte of a malformed text, or the nesting.
nlohmann::json parse(const std::string& text) {
    // A manual scan first, so that a text nested far deeper than 64 levels is rejected before nlohmann::json's own
    // (potentially recursive) parser ever sees it: the limit is a safety bound, not only a reported error.
    std::uint32_t depth = 0;
    bool inString = false;
    bool escaped = false;
    for (const char c : text) {
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }
        if (c == '"') {
            inString = true;
        } else if (c == '{' || c == '[') {
            ++depth;
            if (depth > kMaxDepth) {
                throw JsonException{"json: nested too deeply to decode (more than 64 levels)"};
            }
        } else if ((c == '}' || c == ']') && depth > 0) {
            --depth;
        }
    }
    try {
        return nlohmann::json::parse(text);
    } catch (const nlohmann::json::parse_error& error) {
        throw JsonException{std::format("json: malformed JSON at byte {}: {}", error.byte, error.what())};
    } catch (const nlohmann::json::exception& error) {
        throw JsonException{std::format("json: number out of range: {}", error.what())};
    }
}

}  // namespace

EncodeResult encode(const EncodeArguments& arguments) {
    EncodeCursor cursor{arguments};
    return EncodeResult{.text = cursor.next(0)};
}

DecodeResult decode(const DecodeArguments& arguments) {
    DecodeResult result;
    flatten(parse(arguments.text), result, 0);
    return result;
}

}  // namespace rtype::plugins::json::detail
