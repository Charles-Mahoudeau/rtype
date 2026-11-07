/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleText
*/

#include "CapsuleText.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <ios>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/ComponentValues.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/String.hpp"

namespace rtype::plugins::capsule::format {
namespace {
// --- Reading ---------------------------------------------------------------------------------------------------------

/// @brief What a token is.
enum class TokenKind : std::uint8_t {
    kLeftBracket,   ///< `[`
    kRightBracket,  ///< `]`
    kLeftBrace,     ///< `{`
    kRightBrace,    ///< `}`
    kLeftParen,     ///< `(`
    kRightParen,    ///< `)`
    kEquals,        ///< `=`
    kComma,         ///< `,`
    kName,          ///< A name: letters, digits, `_` and `.`, not starting with a digit.
    kString,        ///< `"text"`, its escapes resolved.
    kNumber,        ///< A number, as written.
    kEnd,           ///< The end of the text.
};

/// @brief One token, and where it starts.
struct Token {
    TokenKind kind{TokenKind::kEnd};  ///< What it is.
    std::string text;                 ///< Its text: a name, a string's content or a number.
    std::size_t line{1};              ///< Its line, from 1.
    std::size_t column{1};            ///< Its column, from 1.
};

/// @return How a token is named in a message.
std::string describe(const Token& token) {
    switch (token.kind) {
        case TokenKind::kName:
            return std::format("'{}'", token.text);
        case TokenKind::kString:
            return std::format("\"{}\"", token.text);
        case TokenKind::kNumber:
            return token.text;
        case TokenKind::kEnd:
            return "the end of the file";
        default:
            return std::format("'{}'", token.text);
    }
}

/// @brief Splits a capsule file into tokens.
class Lexer {
  public:
    Lexer(std::string_view path, std::string_view text) noexcept : _path{path}, _text{text} {}
    ~Lexer() = default;
    Lexer(const Lexer& other) = delete;
    Lexer& operator=(const Lexer& other) = delete;
    Lexer(Lexer&& other) = delete;
    Lexer& operator=(Lexer&& other) = delete;

    /// @return Every token of the text, ending with kEnd.
    /// @throws exceptions::CapsuleException On a character no token starts with, or a string without its end.
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        for (;;) {
            skipBlanks();
            Token token{.kind = TokenKind::kEnd, .text = {}, .line = _line, .column = _column};
            if (_position >= _text.size()) {
                tokens.push_back(std::move(token));
                return tokens;
            }
            const char current = _text.at(_position);
            if (const auto punctuation = punctuationOf(current)) {
                token.kind = *punctuation;
                token.text = std::string(1, current);
                advance();
            } else if (current == '"') {
                token.kind = TokenKind::kString;
                token.text = readString(token);
            } else if (current == '-' || current == '+' || std::isdigit(static_cast<unsigned char>(current)) != 0) {
                token.kind = TokenKind::kNumber;
                token.text = readWhile([](char next) {
                    return std::isalnum(static_cast<unsigned char>(next)) != 0 || next == '.' || next == '-' ||
                           next == '+';
                });
            } else if (std::isalpha(static_cast<unsigned char>(current)) != 0 || current == '_') {
                token.kind = TokenKind::kName;
                token.text = readWhile([](char next) {
                    return std::isalnum(static_cast<unsigned char>(next)) != 0 || next == '_' || next == '.';
                });
            } else {
                throw exceptions::CapsuleException{
                    std::format("{}:{}:{}: unexpected character '{}'", _path, _line, _column, current)};
            }
            tokens.push_back(std::move(token));
        }
    }

  private:
    /// @return The punctuation token a character is, if it is one.
    static std::optional<TokenKind> punctuationOf(char character) noexcept {
        switch (character) {
            case '[':
                return TokenKind::kLeftBracket;
            case ']':
                return TokenKind::kRightBracket;
            case '{':
                return TokenKind::kLeftBrace;
            case '}':
                return TokenKind::kRightBrace;
            case '(':
                return TokenKind::kLeftParen;
            case ')':
                return TokenKind::kRightParen;
            case '=':
                return TokenKind::kEquals;
            case ',':
                return TokenKind::kComma;
            default:
                return std::nullopt;
        }
    }

    /// @return The character an escape (`\n`, `\t`, `\"`, `\\`) stands for.
    static char unescape(char escaped) noexcept {
        switch (escaped) {
            case 'n':
                return '\n';
            case 't':
                return '\t';
            default:
                return escaped;
        }
    }

    /// @brief Moves past one character, counting lines.
    void advance() noexcept {
        if (_text.at(_position) == '\n') {
            ++_line;
            _column = 1;
        } else {
            ++_column;
        }
        ++_position;
    }

    /// @brief Moves past spaces, line ends and `#` comments.
    void skipBlanks() noexcept {
        while (_position < _text.size()) {
            const char current = _text.at(_position);
            if (current == '#') {
                while (_position < _text.size() && _text.at(_position) != '\n') {
                    advance();
                }
            } else if (std::isspace(static_cast<unsigned char>(current)) != 0) {
                advance();
            } else {
                return;
            }
        }
    }

    /// @return The characters from here that `accept` takes.
    template <typename Accept>
    std::string readWhile(Accept accept) {
        const std::size_t start = _position;
        while (_position < _text.size() && accept(_text.at(_position))) {
            advance();
        }
        return std::string{_text.substr(start, _position - start)};
    }

    /// @return A string's content, its escapes (`\"`, `\\`, `\n`, `\t`) resolved.
    std::string readString(const Token& start) {
        advance();  // the opening quote
        std::string content;
        while (_position < _text.size() && _text.at(_position) != '"' && _text.at(_position) != '\n') {
            char current = _text.at(_position);
            if (current == '\\' && _position + 1 < _text.size()) {
                advance();
                current = unescape(_text.at(_position));
            }
            content += current;
            advance();
        }
        if (_position >= _text.size() || _text.at(_position) != '"') {
            throw exceptions::CapsuleException{
                std::format("{}:{}:{}: this string has no closing quote", _path, start.line, start.column)};
        }
        advance();  // the closing quote
        return content;
    }

    std::string_view _path;    ///< The file, for messages.
    std::string_view _text;    ///< The text.
    std::size_t _position{0};  ///< The next character.
    std::size_t _line{1};      ///< The line of the next character.
    std::size_t _column{1};    ///< The column of the next character.
};

/// @brief A value as written, before it is known what field it fills.
struct Value {
    enum class Kind : std::uint8_t { kNumber, kBool, kString, kTuple, kList };

    Kind kind{Kind::kNumber};  ///< What it is.
    std::string text;          ///< A number as written, a string's content, "true" or "false".
    std::vector<Value> items;  ///< A tuple's numbers or a list's values.
    Token where;               ///< Where it starts, for messages.
};

/// @return How a value is named in a message.
std::string describe(const Value& value) {
    switch (value.kind) {
        case Value::Kind::kNumber:
            return "a number";
        case Value::Kind::kBool:
            return "a boolean";
        case Value::Kind::kString:
            return "a string";
        case Value::Kind::kTuple:
            return std::format("a vector of {}", value.items.size());
        case Value::Kind::kList:
            return "a list";
    }
    return "a value";
}

/// @brief Reads the tokens of one capsule file into a capsule.
class Reader {
  public:
    Reader(std::string_view path, std::vector<Token> tokens, const ecs::Components& components, Capsules& capsules,
           const InstanceLoader& loadInstance)
        : _path{path},
          _tokens{std::move(tokens)},
          _components{&components},
          _capsules{&capsules},
          _loadInstance{&loadInstance},
          _capsule{CapsuleId::fromPath(path), std::string{path}} {}
    ~Reader() = default;
    Reader(const Reader& other) = delete;
    Reader& operator=(const Reader& other) = delete;
    Reader(Reader&& other) = delete;
    Reader& operator=(Reader&& other) = delete;

    /// @return The capsule the file describes.
    Capsule read() {
        bool header = false;
        bool anyNode = false;
        while (peek().kind != TokenKind::kEnd) {
            const Token& open = expect(TokenKind::kLeftBracket, "'[' to start a section");
            const Token section = expect(TokenKind::kName, "'capsule' or 'node'");
            const std::vector<std::pair<Token, Token>> attributes = readAttributes();
            expect(TokenKind::kRightBracket, "']' to end the section");
            if (section.text == "capsule") {
                if (header || anyNode) {
                    fail(open, "the [capsule] section comes once, first");
                }
                header = true;
                readHeader(attributes);
            } else if (section.text == "node") {
                readNode(open, attributes, anyNode);
                anyNode = true;
            } else {
                fail(section, std::format("unknown section '{}': expected 'capsule' or 'node'", section.text));
            }
        }
        return std::move(_capsule);
    }

  private:
    /// @return The next token, left there.
    [[nodiscard]] const Token& peek() const { return _tokens.at(_next); }

    /// @return The next token, taken.
    const Token& take() { return _tokens.at(_next++); }

    /// @return The next token, which must be of `kind`.
    const Token& expect(TokenKind kind, std::string_view what) {
        if (peek().kind != kind) {
            fail(peek(), std::format("expected {}, found {}", what, describe(peek())));
        }
        return take();
    }

    /// @brief Throws a CapsuleException at `where`.
    [[noreturn]] void fail(const Token& where, std::string_view message) const {
        throw exceptions::CapsuleException{std::format("{}:{}:{}: {}", _path, where.line, where.column, message)};
    }

    /// @return The `name=value` attributes of a section.
    std::vector<std::pair<Token, Token>> readAttributes() {
        std::vector<std::pair<Token, Token>> attributes;
        while (peek().kind == TokenKind::kName) {
            Token name = take();
            expect(TokenKind::kEquals, std::format("'=' after '{}'", name.text));
            const Token& value = take();
            if (value.kind != TokenKind::kString && value.kind != TokenKind::kName &&
                value.kind != TokenKind::kNumber) {
                fail(value, std::format("expected a value for '{}', found {}", name.text, describe(value)));
            }
            attributes.emplace_back(std::move(name), value);
        }
        return attributes;
    }

    /// @brief Checks the [capsule] section: `format`, the version this reader knows.
    void readHeader(const std::vector<std::pair<Token, Token>>& attributes) const {
        for (const auto& [name, value] : attributes) {
            if (name.text != "format") {
                fail(name, std::format("unknown attribute '{}': [capsule] takes 'format'", name.text));
            }
            if (value.text != std::to_string(kFormatVersion)) {
                fail(value,
                     std::format("format {} is not known: this engine reads format {}", value.text, kFormatVersion));
            }
        }
    }

    /// @brief Reads a [node] section and its components into the capsule.
    void readNode(const Token& open, const std::vector<std::pair<Token, Token>>& attributes, bool anyNode) {
        const Token* path = nullptr;
        const Token* instance = nullptr;
        const Token* side = nullptr;
        for (const auto& [name, value] : attributes) {
            const Token** slot = nullptr;
            if (name.text == "path") {
                slot = &path;
            } else if (name.text == "instance") {
                slot = &instance;
            } else if (name.text == "side") {
                slot = &side;
            }
            if (slot == nullptr) {
                fail(name,
                     std::format("unknown attribute '{}': [node] takes 'path', 'instance' and 'side'", name.text));
            }
            *slot = &value;
        }
        if (path == nullptr || path->kind != TokenKind::kString) {
            fail(open, R"(a [node] needs a path: path="." for the root, path="Boss/Turret" below it)");
        }
        const std::uint32_t index = addNode(*path, anyNode);
        CapsuleNode& node = _capsule.editNode(index);
        if (instance != nullptr) {
            node.instance = instanceOf(*instance);
        }
        if (side != nullptr) {
            if (index == 0) {
                fail(*side, "the root is spawned on every side: it takes no 'side'");
            }
            node.side = sideOf(*side);
        }
        const ComponentValues* base = baseOf(node);
        while (peek().kind == TokenKind::kName) {
            readComponent(node, base);
        }
    }

    /// @return The index of the node a section's path names, added under its parent.
    std::uint32_t addNode(const Token& path, bool anyNode) {
        if (path.text == "." || path.text.empty()) {
            if (anyNode) {
                fail(path, "the root (path=\".\") comes once, first");
            }
            return 0;
        }
        if (!anyNode) {
            fail(path, R"(the first node is the root: path=".")");
        }
        const std::size_t slash = path.text.rfind('/');
        const std::string parentPath = slash == std::string::npos ? std::string{"."} : path.text.substr(0, slash);
        const std::string name = slash == std::string::npos ? path.text : path.text.substr(slash + 1);
        const std::optional<std::uint32_t> parent = _capsule.findNode(parentPath);
        if (!parent) {
            fail(path, std::format("'{}' has no parent node '{}' above it", path.text, parentPath));
        }
        try {
            return _capsule.addNode(name, *parent);
        } catch (const exceptions::CapsuleException& error) {
            fail(path, error.what());
        }
    }

    /// @return The capsule an `instance` attribute names, read first by the loader if it can.
    [[nodiscard]] CapsuleRef instanceOf(const Token& instance) const {
        if (instance.kind != TokenKind::kString) {
            fail(instance, "'instance' takes the path of a capsule, as a string");
        }
        std::string path = core::asset::normalizePath(instance.text);
        if (*_loadInstance) {
            (*_loadInstance)(path);
        }
        return CapsuleRef{.id = CapsuleId::fromPath(path), .path = std::move(path)};
    }

    /// @return The side a `side` attribute names.
    [[nodiscard]] ecs::Side sideOf(const Token& side) const {
        if (side.text == "server") {
            return ecs::Side::kServer;
        }
        if (side.text == "client") {
            return ecs::Side::kClient;
        }
        if (side.text == "both") {
            return ecs::Side::kBoth;
        }
        fail(side, std::format("unknown side '{}': expected server, client or both", side.text));
    }

    /// @return The values an instance node's fields start from (the instanced root's), or nullptr.
    [[nodiscard]] const ComponentValues* baseOf(const CapsuleNode& node) const {
        if (!node.instance) {
            return nullptr;
        }
        const Capsule* instanced = _capsules->find(node.instance->id);
        return instanced != nullptr ? &instanced->getNodes().front().values : nullptr;
    }

    /// @brief Reads `name = { field = value, ... }` into the node.
    void readComponent(CapsuleNode& node, const ComponentValues* base) {
        const Token name = take();
        const ecs::ComponentInfo* info = _components->getRegistry().findByName(name.text);
        if (info == nullptr) {
            fail(name, std::format("unknown component '{}'", name.text));
        }
        if (node.values.has(info->type.id)) {
            fail(name, std::format("component '{}' is set twice on this node", name.text));
        }
        expect(TokenKind::kEquals, std::format("'=' after '{}'", name.text));
        expect(TokenKind::kLeftBrace, std::format("'{{' to start the fields of '{}'", name.text));
        ecs::storage::ByteColumn column{info->type};
        column.reserve(1);
        const std::span<const std::byte> start =
            base != nullptr ? base->get(info->type.id) : std::span<const std::byte>{};
        const std::span<std::byte> bytes =
            base != nullptr && base->has(info->type.id) ? column.emplaceBackCopy(start) : column.emplaceBack();
        std::vector<std::string> seen;
        while (peek().kind != TokenKind::kRightBrace) {
            const Token field = expect(TokenKind::kName, std::format("a field of '{}' or '}}'", name.text));
            if (std::ranges::find(seen, field.text) != seen.end()) {
                fail(field, std::format("field '{}' is set twice", field.text));
            }
            seen.push_back(field.text);
            expect(TokenKind::kEquals, std::format("'=' after '{}'", field.text));
            const Value value = readValue();
            const auto described = std::ranges::find(info->type.fields, field.text, &ecs::FieldInfo::name);
            if (described == info->type.fields.end()) {
                fail(field, std::format("component '{}' has no field '{}'", name.text, field.text));
            }
            writeField(*described, bytes, value, name.text);
            if (peek().kind != TokenKind::kComma) {
                break;
            }
            take();
        }
        expect(TokenKind::kRightBrace, std::format("',' or '}}' to end the fields of '{}'", name.text));
        try {
            node.values.set(*_components, info->type.id, bytes);
        } catch (const exceptions::CapsuleException& error) {
            fail(name, error.what());
        }
    }

    /// @return The value at the next token.
    // NOLINTNEXTLINE(misc-no-recursion): the depth is the nesting of the lists written in the file.
    Value readValue() {
        const Token& start = take();
        Value value{.kind = Value::Kind::kNumber, .text = start.text, .items = {}, .where = start};
        switch (start.kind) {
            case TokenKind::kNumber:
                return value;
            case TokenKind::kString:
                value.kind = Value::Kind::kString;
                return value;
            case TokenKind::kName:
                if (start.text != "true" && start.text != "false") {
                    fail(start, std::format("expected a value, found {}", describe(start)));
                }
                value.kind = Value::Kind::kBool;
                return value;
            case TokenKind::kLeftParen:
            case TokenKind::kLeftBracket: {
                const bool tuple = start.kind == TokenKind::kLeftParen;
                const TokenKind close = tuple ? TokenKind::kRightParen : TokenKind::kRightBracket;
                value.kind = tuple ? Value::Kind::kTuple : Value::Kind::kList;
                while (peek().kind != close) {
                    value.items.push_back(readValue());  // NOLINT(misc-no-recursion)
                    if (peek().kind != TokenKind::kComma) {
                        break;
                    }
                    take();
                }
                expect(close, tuple ? "',' or ')'" : "',' or ']'");
                if (tuple && std::ranges::any_of(value.items,
                                                 [](const Value& item) { return item.kind != Value::Kind::kNumber; })) {
                    fail(start, "a vector holds numbers only: (x, y)");
                }
                return value;
            }
            default:
                fail(start, std::format("expected a value, found {}", describe(start)));
        }
    }

    /// @brief Writes a value into a field of a component's bytes.
    void writeField(const ecs::FieldInfo& field, std::span<std::byte> bytes, const Value& value,
                    std::string_view component) {
        const std::size_t size = ecs::getFieldLayout(field.type).size;
        if (field.list != nullptr) {
            void* list = bytes.subspan(field.offset).data();
            if (field.type == ecs::FieldType::kText) {
                requireKind(value, Value::Kind::kString, component, field);
                field.list->assign(list, std::as_bytes(std::span{value.text}));
                return;
            }
            requireKind(value, Value::Kind::kList, component, field);
            std::vector<std::byte> elements(value.items.size() * size);
            for (std::size_t index = 0; index < value.items.size(); ++index) {
                writeScalar(field, std::span{elements}.subspan(index * size, size), value.items.at(index), component);
            }
            field.list->assign(list, elements);
            return;
        }
        if (field.count > 1) {
            requireKind(value, Value::Kind::kList, component, field);
            if (value.items.size() != field.count) {
                fail(value.where, std::format("field '{}' of '{}' holds {} values, not {}", field.name, component,
                                              field.count, value.items.size()));
            }
            for (std::size_t index = 0; index < field.count; ++index) {
                writeScalar(field, bytes.subspan(field.offset + (index * size), size), value.items.at(index),
                            component);
            }
            return;
        }
        writeScalar(field, bytes.subspan(field.offset, size), value, component);
    }

    /// @brief Fails unless `value` is of `kind`.
    void requireKind(const Value& value, Value::Kind kind, std::string_view component,
                     const ecs::FieldInfo& field) const {
        if (value.kind != kind) {
            const Value expected{.kind = kind, .text = {}, .items = {}, .where = {}};
            fail(value.where, std::format("field '{}' of '{}' takes {}, not {}", field.name, component,
                                          describe(expected), describe(value)));
        }
    }

    /// @brief Writes one value of a field's type into `out`.
    void writeScalar(const ecs::FieldInfo& field, std::span<std::byte> out, const Value& value,
                     std::string_view component) {
        switch (field.type) {
            case ecs::FieldType::kBool: {
                requireKind(value, Value::Kind::kBool, component, field);
                const bool flag = value.text == "true";
                std::memcpy(out.data(), &flag, sizeof(flag));
                return;
            }
            case ecs::FieldType::kI8:
                writeInteger<std::int8_t>(field, out, value, component);
                return;
            case ecs::FieldType::kI16:
                writeInteger<std::int16_t>(field, out, value, component);
                return;
            case ecs::FieldType::kI32:
                writeInteger<std::int32_t>(field, out, value, component);
                return;
            case ecs::FieldType::kI64:
                writeInteger<std::int64_t>(field, out, value, component);
                return;
            case ecs::FieldType::kU8:
                writeInteger<std::uint8_t>(field, out, value, component);
                return;
            case ecs::FieldType::kU16:
                writeInteger<std::uint16_t>(field, out, value, component);
                return;
            case ecs::FieldType::kU32:
                writeInteger<std::uint32_t>(field, out, value, component);
                return;
            case ecs::FieldType::kU64:
                writeInteger<std::uint64_t>(field, out, value, component);
                return;
            case ecs::FieldType::kF32: {
                const auto number = static_cast<float>(readFloat(field, value, component));
                std::memcpy(out.data(), &number, sizeof(number));
                return;
            }
            case ecs::FieldType::kF64: {
                const double number = readFloat(field, value, component);
                std::memcpy(out.data(), &number, sizeof(number));
                return;
            }
            case ecs::FieldType::kVec2:
            case ecs::FieldType::kVec3:
            case ecs::FieldType::kVec4:
                writeVector(field, out, value, component);
                return;
            case ecs::FieldType::kAssetId:
                writeAsset(field, out, value, component);
                return;
            case ecs::FieldType::kString: {
                requireKind(value, Value::Kind::kString, component, field);
                if (value.text.size() > 255) {
                    fail(value.where, std::format("field '{}' of '{}' takes 255 bytes at most", field.name, component));
                }
                const ecs::String text{std::string_view{value.text}};
                std::memcpy(out.data(), &text, sizeof(text));
                return;
            }
            case ecs::FieldType::kEntity:
            case ecs::FieldType::kText:
                break;
        }
        fail(value.where, std::format("field '{}' of '{}' cannot be set in a capsule file yet", field.name, component));
    }

    /// @brief Writes an integer, checked to fit `T`.
    template <typename T>
    void writeInteger(const ecs::FieldInfo& field, std::span<std::byte> out, const Value& value,
                      std::string_view component) const {
        requireKind(value, Value::Kind::kNumber, component, field);
        const std::optional<T> number = parseInteger<T>(value.text);
        if (!number) {
            fail(value.where,
                 std::format("field '{}' of '{}' takes a whole number from {} to {}, not {}", field.name, component,
                             +std::numeric_limits<T>::min(), +std::numeric_limits<T>::max(), value.text));
        }
        std::memcpy(out.data(), &*number, sizeof(T));
    }

    /// @return The integer `text` writes (decimal, or hexadecimal after `0x`), or nothing if it does not fit `T`.
    template <typename T>
    static std::optional<T> parseInteger(std::string_view text) {
        int base = 10;
        if (text.starts_with("0x") || text.starts_with("0X")) {
            base = 16;
            text.remove_prefix(2);
        }
        if (text.starts_with('+')) {
            text.remove_prefix(1);
        }
        T number{};
        const char* first = std::to_address(text.begin());
        const char* last = std::to_address(text.end());
        const auto [end, error] = std::from_chars(first, last, number, base);
        if (error != std::errc{} || end != last || text.empty()) {
            return std::nullopt;
        }
        return number;
    }

    /// @return The number a value writes.
    [[nodiscard]] double readFloat(const ecs::FieldInfo& field, const Value& value, std::string_view component) const {
        requireKind(value, Value::Kind::kNumber, component, field);
        const std::optional<double> number = parseFloat(value.text);
        if (!number) {
            fail(value.where,
                 std::format("field '{}' of '{}' takes a number, not {}", field.name, component, value.text));
        }
        return *number;
    }

    /// @return The finite number `text` writes, or nothing.
    static std::optional<double> parseFloat(const std::string& text) {
        if (text.contains('x') || text.contains('X')) {
            return std::nullopt;  // no hexadecimal floats
        }
        char* end = nullptr;
        const double number = std::strtod(text.c_str(), &end);
        if (end != std::to_address(text.end()) || !std::isfinite(number)) {
            return std::nullopt;
        }
        return number;
    }

    /// @brief Writes `(x, y[, z[, w]])` into a vector field.
    void writeVector(const ecs::FieldInfo& field, std::span<std::byte> out, const Value& value,
                     std::string_view component) const {
        requireKind(value, Value::Kind::kTuple, component, field);
        const std::size_t count = out.size() / sizeof(float);
        if (value.items.size() != count) {
            fail(value.where, std::format("field '{}' of '{}' takes a vector of {}, not {}", field.name, component,
                                          count, value.items.size()));
        }
        for (std::size_t index = 0; index < count; ++index) {
            const auto number = static_cast<float>(readFloat(field, value.items.at(index), component));
            std::memcpy(out.subspan(index * sizeof(float)).data(), &number, sizeof(number));
        }
    }

    /// @brief Writes an asset id: a path (remembered), or a number for an id with no path.
    void writeAsset(const ecs::FieldInfo& field, std::span<std::byte> out, const Value& value,
                    std::string_view component) {
        core::asset::AssetId id;
        if (value.kind == Value::Kind::kString) {
            std::string path = core::asset::normalizePath(value.text);
            id = core::asset::AssetId::fromPath(path);
            _capsules->rememberAssetPath(id, std::move(path));
        } else {
            requireKind(value, Value::Kind::kNumber, component, field);
            const std::optional<std::uint64_t> raw = parseInteger<std::uint64_t>(value.text);
            if (!raw) {
                fail(value.where, std::format("field '{}' of '{}' takes an asset path", field.name, component));
            }
            id.value = *raw;
        }
        std::memcpy(out.data(), &id, sizeof(id));
    }

    std::string_view _path;               ///< The file, for messages.
    std::vector<Token> _tokens;           ///< Its tokens.
    std::size_t _next{0};                 ///< The next token.
    const ecs::Components* _components;   ///< The names a file uses.
    Capsules* _capsules;                  ///< Instanced capsules, and the asset paths read.
    const InstanceLoader* _loadInstance;  ///< Reads an instanced capsule first.
    Capsule _capsule;                     ///< What the file describes.
};

// --- Writing ---------------------------------------------------------------------------------------------------------

/// @return `text` as a quoted string, escaped.
std::string quoteText(std::string_view text) {
    std::string out = "\"";
    for (const char character : text) {
        if (character == '"' || character == '\\') {
            out += '\\';
            out += character;
        } else if (character == '\n') {
            out += "\\n";
        } else if (character == '\t') {
            out += "\\t";
        } else {
            out += character;
        }
    }
    return out + '"';
}

/// @return Whether `text` reads back as `value` exactly.
template <typename T>
bool readsBack(const std::string& text, T value) {
    return static_cast<T>(std::strtod(text.c_str(), nullptr)) == value;
}

/// @return The shortest decimal text that reads back as `value` exactly: plain decimals (`-40`, `0.1`) for the usual
/// magnitudes, an exponent only for very large or very small numbers.
template <typename T>
std::string formatFloat(T value) {
    const double magnitude = std::fabs(static_cast<double>(value));
    const bool plain = magnitude == 0.0 || (magnitude >= 1e-4 && magnitude < 1e15);
    for (int precision = plain ? 0 : 1; precision <= std::numeric_limits<T>::max_digits10 + 4; ++precision) {
        std::ostringstream stream;
        stream.imbue(std::locale::classic());
        if (plain) {
            stream << std::fixed;
        }
        stream.precision(precision);
        stream << static_cast<double>(value);
        const std::string text = stream.str();
        if (readsBack(text, value)) {
            return text == "-0" ? std::string{"0"} : text;
        }
    }
    return std::to_string(value);
}

/// @brief Writes the values of a component as file text.
class Writer {
  public:
    explicit Writer(const Capsules& capsules) noexcept : _capsules{&capsules} {}
    ~Writer() = default;
    Writer(const Writer& other) = delete;
    Writer& operator=(const Writer& other) = delete;
    Writer(Writer&& other) = delete;
    Writer& operator=(Writer&& other) = delete;

    /// @return `name = { ... }` with the fields of `value` that differ from `base`.
    [[nodiscard]] std::string component(const ecs::ComponentInfo& info, std::span<const std::byte> value,
                                        std::span<const std::byte> base) const {
        std::vector<std::string> fields;
        for (const ecs::FieldInfo& field : info.type.fields) {
            if (field.type == ecs::FieldType::kEntity || same(field, value, base)) {
                continue;
            }
            fields.push_back(std::format("{} = {}", field.name, this->field(field, value)));
        }
        if (fields.empty()) {
            return std::format("{} = {{}}\n", info.type.name);
        }
        std::string joined;
        for (const std::string& text : fields) {
            joined += joined.empty() ? text : std::format(", {}", text);
        }
        return std::format("{} = {{ {} }}\n", info.type.name, joined);
    }

  private:
    /// @return Whether a field holds the same values in both.
    static bool same(const ecs::FieldInfo& field, std::span<const std::byte> value, std::span<const std::byte> base) {
        if (field.list != nullptr) {
            const std::span<const std::byte> left = field.list->elements(value.subspan(field.offset).data());
            const std::span<const std::byte> right = field.list->elements(base.subspan(field.offset).data());
            return std::ranges::equal(left, right);
        }
        const std::size_t size = static_cast<std::size_t>(ecs::getFieldLayout(field.type).size) * field.count;
        return std::ranges::equal(value.subspan(field.offset, size), base.subspan(field.offset, size));
    }

    /// @return A field's value as file text.
    [[nodiscard]] std::string field(const ecs::FieldInfo& field, std::span<const std::byte> value) const {
        const std::size_t size = ecs::getFieldLayout(field.type).size;
        if (field.list != nullptr) {
            const std::span<const std::byte> elements = field.list->elements(value.subspan(field.offset).data());
            if (field.type == ecs::FieldType::kText) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the bytes of a std::string are its text
                return quoteText(std::string_view{reinterpret_cast<const char*>(elements.data()), elements.size()});
            }
            return list(field.type, elements, elements.size() / size);
        }
        if (field.count > 1) {
            return list(field.type, value.subspan(field.offset, size * field.count), field.count);
        }
        return scalar(field.type, value.subspan(field.offset, size));
    }

    /// @return `[a, b, ...]`.
    [[nodiscard]] std::string list(ecs::FieldType type, std::span<const std::byte> values, std::size_t count) const {
        const std::size_t size = ecs::getFieldLayout(type).size;
        std::string out = "[";
        for (std::size_t index = 0; index < count; ++index) {
            out += index == 0 ? "" : ", ";
            out += scalar(type, values.subspan(index * size, size));
        }
        return out + "]";
    }

    /// @return One value as file text.
    [[nodiscard]] std::string scalar(ecs::FieldType type, std::span<const std::byte> bytes) const {
        switch (type) {
            case ecs::FieldType::kBool:
                return read<bool>(bytes) ? "true" : "false";
            case ecs::FieldType::kI8:
                return std::to_string(read<std::int8_t>(bytes));
            case ecs::FieldType::kI16:
                return std::to_string(read<std::int16_t>(bytes));
            case ecs::FieldType::kI32:
                return std::to_string(read<std::int32_t>(bytes));
            case ecs::FieldType::kI64:
                return std::to_string(read<std::int64_t>(bytes));
            case ecs::FieldType::kU8:
                return std::to_string(read<std::uint8_t>(bytes));
            case ecs::FieldType::kU16:
                return std::to_string(read<std::uint16_t>(bytes));
            case ecs::FieldType::kU32:
                return std::to_string(read<std::uint32_t>(bytes));
            case ecs::FieldType::kU64:
                return std::to_string(read<std::uint64_t>(bytes));
            case ecs::FieldType::kF32:
                return formatFloat(read<float>(bytes));
            case ecs::FieldType::kF64:
                return formatFloat(read<double>(bytes));
            case ecs::FieldType::kVec2:
            case ecs::FieldType::kVec3:
            case ecs::FieldType::kVec4: {
                std::string out = "(";
                for (std::size_t index = 0; index < bytes.size() / sizeof(float); ++index) {
                    out += index == 0 ? "" : ", ";
                    out += formatFloat(read<float>(bytes.subspan(index * sizeof(float))));
                }
                return out + ")";
            }
            case ecs::FieldType::kAssetId: {
                const auto id = read<core::asset::AssetId>(bytes);
                if (const std::string* path = _capsules->findAssetPath(id)) {
                    return quoteText(*path);
                }
                return std::format("{:#018x}", id.value);
            }
            case ecs::FieldType::kString:
                return quoteText(read<ecs::String>(bytes).view());
            case ecs::FieldType::kEntity:
            case ecs::FieldType::kText:
                break;
        }
        return "{}";
    }

    /// @return The value of type `T` at the start of `bytes`.
    template <typename T>
    static T read(std::span<const std::byte> bytes) {
        T value{};
        std::memcpy(&value, bytes.data(), sizeof(T));
        return value;
    }

    const Capsules* _capsules;  ///< The asset paths read.
};

/// @return How a side is written.
std::string_view sideName(ecs::Side side) noexcept {
    switch (side) {
        case ecs::Side::kClient:
            return "client";
        case ecs::Side::kBoth:
            return "both";
        case ecs::Side::kServer:
            break;
    }
    return "server";
}
}  // namespace

bool isCapsuleFile(const std::filesystem::path& path) {
    const std::string extension = path.extension().string();
    return std::ranges::find(kExtensions, extension) != kExtensions.end();
}

Capsule readCapsule(std::string_view path, std::string_view text, const ecs::Components& components, Capsules& capsules,
                    const InstanceLoader& loadInstance) {
    Lexer lexer{path, text};
    Reader reader{path, lexer.tokenize(), components, capsules, loadInstance};
    return reader.read();
}

std::string writeCapsule(const Capsule& capsule, const ecs::Components& components, const Capsules& capsules) {
    const Writer writer{capsules};
    std::string out = std::format("[capsule format={}]\n", kFormatVersion);
    const auto nodes = capsule.getNodes();
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const CapsuleNode& node = *std::next(nodes.begin(), static_cast<std::ptrdiff_t>(index));
        out += std::format("\n[node path={}", quoteText(capsule.getNodePath(index)));
        if (node.instance) {
            out += std::format(" instance={}", quoteText(node.instance->path));
        }
        if (index != 0 && node.side != ecs::Side::kServer) {
            out += std::format(" side={}", sideName(node.side));
        }
        out += "]\n";
        const Capsule* instanced = node.instance ? capsules.find(node.instance->id) : nullptr;
        const ComponentValues* base = instanced != nullptr ? &instanced->getNodes().front().values : nullptr;
        for (std::size_t value = 0; value < node.values.getCount(); ++value) {
            const ecs::ComponentInfo* info = components.getRegistry().findById(node.values.getId(value));
            if (info == nullptr) {
                continue;
            }
            ecs::storage::ByteColumn defaults{info->type};
            defaults.reserve(1);
            const std::span<const std::byte> start = base != nullptr && base->has(info->type.id)
                                                         ? base->get(info->type.id)
                                                         : std::span<const std::byte>{defaults.emplaceBack()};
            out += writer.component(*info, node.values.getValue(value), start);
        }
    }
    return out;
}
}  // namespace rtype::plugins::capsule::format
