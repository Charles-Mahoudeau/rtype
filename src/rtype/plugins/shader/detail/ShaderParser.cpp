/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderParser
*/

#include "plugins/shader/detail/ShaderParser.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <locale>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/shader/exceptions/ShaderExceptions.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

namespace rtype::plugins::shader::detail {

namespace {

using contract::renderer::ShaderValueType;

/// @brief Names a shader cannot declare: the engine's built-ins (docs/Shaders.md).
constexpr std::array<std::string_view, 5> kBuiltins{"VERTEX", "UV", "COLOR", "TEXTURE", "TIME"};

/// @brief What the parser knows of a value type: its GLSL name, its components, its std430 size and alignment.
struct TypeInfo {
    std::string_view name;                          ///< As written in GLSL.
    ShaderValueType type{ShaderValueType::kFloat};  ///< Its contract value.
    std::uint32_t components{1};                    ///< Numbers in a default.
    std::uint32_t alignment{4};                     ///< std140 and std430 alignment, in bytes.
};

/// @brief The value types a shader may declare.
constexpr std::array<TypeInfo, 5> kTypes{{
    {.name = "float", .type = ShaderValueType::kFloat, .components = 1, .alignment = 4},
    {.name = "vec2", .type = ShaderValueType::kVec2, .components = 2, .alignment = 8},
    {.name = "vec3", .type = ShaderValueType::kVec3, .components = 3, .alignment = 16},
    {.name = "vec4", .type = ShaderValueType::kVec4, .components = 4, .alignment = 16},
    {.name = "int", .type = ShaderValueType::kInt, .components = 1, .alignment = 4},
}};

/// @brief A value read from a declaration, before it is laid out.
struct Declaration {
    bool instance{false};                     ///< `instance uniform`.
    std::string type;                         ///< The GLSL type.
    std::string name;                         ///< The value's name.
    std::optional<std::string> defaultValue;  ///< The text after `=`, trimmed.
};

/// @brief Throws a ShaderException naming the shader and the line.
[[noreturn]] void fail(std::string_view shader, std::uint32_t line, std::string_view problem) {
    throw exceptions::ShaderException{std::format("Shader '{}', line {}: {}", shader, line, problem)};
}

/// @return @p text without its leading and trailing spaces.
std::string_view trim(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string_view::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}

/// @return Whether @p name is a GLSL identifier.
bool isIdentifier(std::string_view name) noexcept {
    const auto isStart = [](char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_'; };
    const auto isPart = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; };
    return !name.empty() && isStart(name.front()) && std::ranges::all_of(name, isPart);
}

/// @return @p line without its comments, @p inComment telling whether a block comment is still open at its end.
std::string removeComments(std::string_view line, bool& inComment) {
    std::string code;
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (inComment) {
            if (line.substr(i, 2) == "*/") {
                inComment = false;
                ++i;
            }
            continue;
        }
        if (line.substr(i, 2) == "//") {
            break;
        }
        if (line.substr(i, 2) == "/*") {
            inComment = true;
            ++i;
            continue;
        }
        code += line.at(i);
    }
    return code;
}

/// @return The declaration on @p code, already stripped of comments and starting with `uniform` or `instance`.
Declaration readDeclaration(std::string_view code, std::string_view shader, std::uint32_t line) {
    const auto semicolon = code.find(';');
    if (semicolon == std::string_view::npos || !trim(code.substr(semicolon + 1)).empty()) {
        fail(shader, line, "a declaration ends with ';' and is alone on its line");
    }
    std::string_view head = code.substr(0, semicolon);
    Declaration declaration;
    if (const auto equal = head.find('='); equal != std::string_view::npos) {
        declaration.defaultValue = std::string{trim(head.substr(equal + 1))};
        head = head.substr(0, equal);
    }
    std::vector<std::string> words;
    std::istringstream stream{std::string{head}};
    for (std::string word; stream >> word;) {
        words.push_back(std::move(word));
    }
    if (!words.empty() && words.front() == "instance") {
        declaration.instance = true;
        words.erase(words.begin());
    }
    if (words.size() != 3 || words.at(0) != "uniform") {
        fail(shader, line, "expected '[instance] uniform <type> <name> [= <default>];'");
    }
    declaration.type = words.at(1);
    declaration.name = words.at(2);
    if (!isIdentifier(declaration.name)) {
        fail(shader, line, std::format("'{}' is not a name (arrays are not supported)", declaration.name));
    }
    return declaration;
}

/// @return The number written in @p text, read whatever the locale.
std::optional<float> readNumber(std::string_view text) {
    std::istringstream stream{std::string{trim(text)}};
    stream.imbue(std::locale::classic());
    float value = 0.0F;
    if (!(stream >> value) || !stream.eof()) {
        return std::nullopt;
    }
    return value;
}

/// @return The @p components numbers of the default @p text of a @p type: a number, or `type(...)` of one number
/// (every component) or of one number per component.
std::vector<float> readDefault(std::string_view text, const TypeInfo& type, std::string_view shader,
                               std::uint32_t line) {
    std::vector<std::string_view> parts;
    if (text.starts_with(type.name) && trim(text.substr(type.name.size())).starts_with('(') && text.ends_with(')')) {
        std::string_view arguments = trim(text.substr(type.name.size()));
        arguments = arguments.substr(1, arguments.size() - 2);
        for (std::size_t start = 0;;) {
            const auto comma = arguments.find(',', start);
            parts.push_back(arguments.substr(start, comma - start));
            if (comma == std::string_view::npos) {
                break;
            }
            start = comma + 1;
        }
    } else {
        parts.push_back(text);
    }
    std::vector<float> numbers;
    for (const std::string_view part : parts) {
        const std::optional<float> number = readNumber(part);
        if (!number) {
            fail(shader, line, std::format("cannot read '{}' as a default of type {}", text, type.name));
        }
        numbers.push_back(*number);
    }
    if (numbers.size() == 1) {
        numbers.resize(type.components, numbers.front());
    }
    if (numbers.size() != type.components) {
        fail(shader, line, std::format("a {} default has {} numbers", type.name, type.components));
    }
    return numbers;
}

/// @brief Writes @p numbers, of @p type, at @p offset of @p block: floats, or one int32.
void writeDefault(std::span<std::byte> block, std::uint32_t offset, const TypeInfo& type,
                  const std::vector<float>& numbers) {
    if (type.type == ShaderValueType::kInt) {
        const auto value = static_cast<std::int32_t>(numbers.front());
        std::memcpy(block.subspan(offset, sizeof(value)).data(), &value, sizeof(value));
        return;
    }
    std::memcpy(block.subspan(offset, numbers.size() * sizeof(float)).data(), numbers.data(),
                numbers.size() * sizeof(float));
}

/// @return The value type named @p name, or nullptr if it is not one.
const TypeInfo* findType(std::string_view name) noexcept {
    for (const TypeInfo& type : kTypes) {
        if (type.name == name) {
            return &type;
        }
    }
    return nullptr;
}

/// @return @p value rounded up to a multiple of @p alignment.
std::uint32_t alignUp(std::uint32_t value, std::uint32_t alignment) noexcept {
    return (value + alignment - 1) / alignment * alignment;
}

/// @return Whether @p code, stripped of comments and with its spaces collapsed, defines `void <function>()`.
bool definesFunction(std::string_view code, std::string_view function) {
    return code.contains(std::format("void {}(", function));
}

/// @return @p code with every run of spaces made one space, and no space before '('.
std::string collapseSpaces(std::string_view code) {
    std::string collapsed;
    for (const char c : code) {
        const bool space = std::isspace(static_cast<unsigned char>(c)) != 0;
        if (space && (collapsed.empty() || collapsed.back() == ' ')) {
            continue;
        }
        if (c == '(' && !collapsed.empty() && collapsed.back() == ' ') {
            collapsed.pop_back();
        }
        collapsed += space ? ' ' : c;
    }
    return collapsed;
}

}  // namespace

namespace {

/// @brief What parseShader() builds while it reads a shader.
struct Parser {
    ParsedShader shader;                   ///< The result.
    std::vector<std::byte> instanceBlock;  ///< The instance values' defaults, kShaderInstanceBytes.
    std::uint32_t materialCursor{0};       ///< End of the material block so far.
    std::uint32_t instanceCursor{0};       ///< End of the instance block so far.
    std::vector<std::string> names;        ///< Every name declared so far.

    /// @brief Adds the texture @p declaration, read on @p line.
    void addTexture(const Declaration& declaration, std::uint32_t line) {
        if (declaration.instance || declaration.defaultValue) {
            fail(shader.name, line, "a texture is a plain 'uniform sampler2D <name>;'");
        }
        if (shader.textures.size() == contract::renderer::kShaderMaxTextures) {
            fail(shader.name, line, std::format("more than {} textures", contract::renderer::kShaderMaxTextures));
        }
        shader.textures.push_back(
            {.name = declaration.name, .slot = static_cast<std::uint32_t>(shader.textures.size())});
    }

    /// @brief Lays out the value @p declaration, read on @p line, in its block, with its default.
    void addValue(const Declaration& declaration, std::uint32_t line) {
        const TypeInfo* type = findType(declaration.type);
        if (type == nullptr) {
            fail(shader.name, line,
                 std::format("type '{}' is not float, vec2, vec3, vec4, int or sampler2D", declaration.type));
        }
        const std::vector<float> numbers = declaration.defaultValue
                                               ? readDefault(*declaration.defaultValue, *type, shader.name, line)
                                               : std::vector<float>(type->components, 0.0F);
        std::uint32_t& cursor = declaration.instance ? instanceCursor : materialCursor;
        const std::uint32_t offset = alignUp(cursor, type->alignment);
        cursor = offset + (type->components * 4);
        const DeclaredValue value{.value = {.name = declaration.name, .type = type->type, .offset = offset},
                                  .line = line};
        if (declaration.instance) {
            if (cursor > contract::renderer::kShaderInstanceBytes) {
                fail(shader.name, line,
                     std::format("the instance values take more than {} bytes",
                                 contract::renderer::kShaderInstanceBytes));
            }
            writeDefault(instanceBlock, offset, *type, numbers);
            shader.instanceValues.push_back(value);
        } else {
            shader.materialDefaults.resize(alignUp(cursor, 16));
            writeDefault(shader.materialDefaults, offset, *type, numbers);
            shader.materialValues.push_back(value);
        }
    }

    /// @brief Reads the declaration on @p code, line @p line.
    void addDeclaration(std::string_view code, std::uint32_t line) {
        const Declaration declaration = readDeclaration(code, shader.name, line);
        if (std::ranges::find(kBuiltins, declaration.name) != kBuiltins.end() ||
            declaration.name.starts_with("rtype") || declaration.name.starts_with("gl_")) {
            fail(shader.name, line, std::format("'{}' is reserved by the engine", declaration.name));
        }
        if (std::ranges::find(names, declaration.name) != names.end()) {
            fail(shader.name, line, std::format("'{}' is declared twice", declaration.name));
        }
        names.push_back(declaration.name);
        if (declaration.type == "sampler2D") {
            addTexture(declaration, line);
        } else {
            addValue(declaration, line);
        }
    }
};

}  // namespace

ParsedShader parseShader(std::string_view source, std::string_view name) {
    Parser parser{.shader = {.name = std::string{name}},
                  .instanceBlock = std::vector<std::byte>(contract::renderer::kShaderInstanceBytes)};
    ParsedShader& shader = parser.shader;
    std::string code;
    bool inComment = false;
    std::uint32_t lineNumber = 0;
    for (std::size_t start = 0; start <= source.size();) {
        const auto end = std::min(source.find('\n', start), source.size());
        const std::string_view line = source.substr(start, end - start);
        start = end + 1;
        ++lineNumber;
        const std::string lineCode = removeComments(line, inComment);
        const std::string_view trimmed = trim(lineCode);
        if (trimmed.starts_with("uniform ") || trimmed.starts_with("instance ")) {
            shader.body += '\n';
            parser.addDeclaration(trimmed, lineNumber);
            continue;
        }
        shader.body += line;
        shader.body += '\n';
        code += lineCode;
        code += '\n';
    }
    shader.body.pop_back();  // the loop added a '\n' the source did not end with
    shader.materialSize = alignUp(parser.materialCursor, 16);
    shader.materialDefaults.resize(shader.materialSize);
    std::memcpy(shader.instanceDefaults.data(), parser.instanceBlock.data(), parser.instanceBlock.size());
    const std::string collapsed = collapseSpaces(code);
    shader.hasVertex = definesFunction(collapsed, "vertex");
    shader.hasFragment = definesFunction(collapsed, "fragment");
    if (!shader.hasVertex && !shader.hasFragment) {
        fail(name, lineNumber, "the shader defines neither 'void vertex()' nor 'void fragment()'");
    }
    return std::move(parser.shader);
}

}  // namespace rtype::plugins::shader::detail
