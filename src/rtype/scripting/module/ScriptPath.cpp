/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptPath
*/

#include "rtype/scripting/module/ScriptPath.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting {
namespace {
[[nodiscard]] char lower(const char c) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

[[nodiscard]] bool endsWithIgnoringCase(const std::string_view text, const std::string_view suffix) noexcept {
    return text.size() >= suffix.size() &&
           std::ranges::equal(text.substr(text.size() - suffix.size()), suffix, {}, lower, lower);
}

/// @brief Splits @p rest on '/' and checks every segment.
[[nodiscard]] Result<std::vector<std::string>> splitSegments(const std::string_view rest) {
    std::vector<std::string> segments;
    std::size_t begin = 0;
    while (begin <= rest.size()) {
        const std::size_t end = std::min(rest.find('/', begin), rest.size());
        const std::string_view segment = rest.substr(begin, end - begin);
        const bool last = end == rest.size();
        if (segment.empty()) {
            return fail(ErrorKind::kInvalidPath,
                        last ? "invalid path: a path cannot end with '/'" : "invalid path: empty segment ('//')");
        }
        if (segment == "." || segment == "..") {
            return fail(ErrorKind::kInvalidPath,
                        std::format("invalid path: '{}' is only allowed at the start of a path", segment));
        }
        if (last && (endsWithIgnoringCase(segment, ".luau") || endsWithIgnoringCase(segment, ".lua"))) {
            return fail(ErrorKind::kInvalidPath, std::format("invalid path: remove the extension of '{}'", segment));
        }
        segments.emplace_back(segment);
        begin = end + 1;
    }
    return segments;
}
}  // namespace

bool ScriptPath::isValidAliasName(const std::string_view name) noexcept {
    return !name.empty() && name != "." && name != ".." && std::ranges::all_of(name, [](const char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '.' || c == '-';
    });
}

Result<ScriptPath> ScriptPath::parse(const std::string_view text) {
    if (text.empty()) {
        return fail(ErrorKind::kInvalidPath, "invalid path: the path is empty");
    }
    if (text.contains('\\')) {
        return fail(ErrorKind::kInvalidPath, "invalid path: use '/' as separator, not '\\'");
    }
    ScriptPath path;
    std::string_view rest = text;
    if (text.starts_with('@')) {
        const std::size_t slash = text.find('/');
        const std::string_view name = text.substr(1, slash == std::string_view::npos ? slash : slash - 1);
        if (!isValidAliasName(name)) {
            return fail(
                ErrorKind::kInvalidPath,
                std::format("invalid path: '{}' is not a valid alias name (use a-z, 0-9, '_', '.', '-')", name));
        }
        path.kind = Kind::kAlias;
        path.alias.resize(name.size());
        std::ranges::transform(name, path.alias.begin(), lower);
        if (slash == std::string_view::npos) {
            return path;
        }
        rest = text.substr(slash + 1);
    } else if (text.starts_with("./")) {
        rest = text.substr(2);
    } else if (text.starts_with("../")) {
        while (rest.starts_with("../")) {
            ++path.parents;
            rest.remove_prefix(3);
        }
    } else {
        return fail(ErrorKind::kInvalidPath,
                    "invalid path: it must start with './', '../' or '@alias' (for example './config' or "
                    "'@scripts/config')");
    }
    auto segments = splitSegments(rest);
    if (!segments) {
        return tl::unexpected{segments.error()};
    }
    path.segments = *std::move(segments);
    return path;
}
}  // namespace rtype::scripting
