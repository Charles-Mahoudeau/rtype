/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptResolver
*/

#include "rtype/scripting/modules/ScriptResolver.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "rtype/luau/ModuleSource.hpp"
#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/Result.hpp"
#include "rtype/scripting/exceptions/ScriptLoadException.hpp"
#include "rtype/scripting/modules/ScriptPath.hpp"

namespace rtype::scripting::modules {
namespace {
using Segments = std::vector<std::string>;

constexpr std::string_view kInit = "init.luau";
constexpr std::string_view kScripts = "scripts";
constexpr std::string_view kSelf = "self";

[[nodiscard]] char lower(const char c) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

[[nodiscard]] bool equalsIgnoringCase(const std::string_view a, const std::string_view b) noexcept {
    return std::ranges::equal(a, b, {}, lower, lower);
}

[[nodiscard]] std::string lowered(const std::string_view text) {
    std::string result{text};
    std::ranges::transform(result, result.begin(), lower);
    return result;
}

[[nodiscard]] std::string join(const Segments& segments) {
    std::string result;
    for (const std::string& segment : segments) {
        result += result.empty() ? "" : "/";
        result += segment;
    }
    return result;
}

[[nodiscard]] std::string displayed(const Segments& segments) {
    return segments.empty() ? std::string{kScripts} : std::format("{}/{}", kScripts, join(segments));
}

/// @brief Outcome of looking for one name in a folder.
struct Lookup {
    enum class Status : unsigned char { kFound, kMissing, kCaseMismatch };
    Status status = Status::kMissing;
    std::string realName;  ///< The name on disk, for a case mismatch.
    bool isFolder = false;
};

/// @brief Looks for @p name in @p directory by exact name: the listing is read, so the answer does not depend on the
/// case rules of the file system.
[[nodiscard]] Lookup lookup(const std::filesystem::path& directory, const std::string_view name, const bool folder) {
    Lookup result;
    result.isFolder = folder;
    std::error_code error;
    std::filesystem::directory_iterator it{directory, std::filesystem::directory_options::skip_permission_denied,
                                           error};
    if (error) {
        return result;
    }
    for (const std::filesystem::directory_iterator end; it != end; it.increment(error)) {
        if (error) {
            break;
        }
        std::error_code typeError;
        const bool fits = folder ? it->is_directory(typeError) : it->is_regular_file(typeError);
        if (typeError || !fits) {
            continue;
        }
        const std::string entryName = it->path().filename().generic_string();
        if (entryName == name) {
            result.status = Lookup::Status::kFound;
            result.realName = entryName;
            return result;
        }
        if (result.status == Lookup::Status::kMissing && equalsIgnoringCase(entryName, name)) {
            result.status = Lookup::Status::kCaseMismatch;
            result.realName = entryName;
        }
    }
    return result;
}

/// @brief Walks @p folders then looks for the file @p fileName, all with their exact names.
[[nodiscard]] Lookup walk(const std::filesystem::path& root, const Segments& folders, const std::string_view fileName) {
    std::filesystem::path current = root;
    for (const std::string& folder : folders) {
        const Lookup found = lookup(current, folder, true);
        if (found.status != Lookup::Status::kFound) {
            return found;
        }
        current /= folder;
    }
    return lookup(current, fileName, false);
}

[[nodiscard]] std::string caseMessage(const Lookup& found) {
    return std::format("case mismatch: the {} is '{}'", found.isFolder ? "folder" : "file", found.realName);
}

/// @brief Splits a requester key into folders, and tells whether the requester is an `init.luau`.
struct Requester {
    Segments folder;      ///< The folder holding the file.
    bool isInit = false;  ///< The file is `init.luau`.
};

[[nodiscard]] Requester splitRequester(const std::string_view key) {
    Requester requester;
    std::size_t begin = 0;
    while (begin < key.size()) {
        const std::size_t end = std::min(key.find('/', begin), key.size());
        requester.folder.emplace_back(key.substr(begin, end - begin));
        begin = end + 1;
    }
    if (!requester.folder.empty()) {
        requester.isInit = requester.folder.back() == kInit;
        requester.folder.pop_back();
    }
    return requester;
}

/// @brief Validates an alias target and gives its segments; an empty result with no error means `scripts/` itself.
[[nodiscard]] std::optional<Segments> targetSegments(const std::filesystem::path& root, const std::string& target) {
    const std::filesystem::path normal = std::filesystem::path{target}.lexically_normal();
    if (target.empty() || target.contains('\\') || normal.has_root_path()) {
        return std::nullopt;
    }
    Segments segments;
    for (const std::filesystem::path& part : normal) {
        const std::string name = part.generic_string();
        if (name == "..") {
            return std::nullopt;
        }
        if (!name.empty() && name != ".") {
            segments.push_back(name);
        }
    }
    if (!core::asset::isInsideDirectory(root, root / normal)) {
        return std::nullopt;
    }
    return segments;
}
[[nodiscard]] std::string unknownAlias(const std::string& name,
                                       const std::map<std::string, std::vector<std::string>>& aliases) {
    std::vector<std::string> known{std::string{kScripts}, std::string{kSelf}};
    for (const auto& entry : aliases) {
        known.push_back(entry.first);
    }
    std::ranges::sort(known);
    std::string list;
    for (const std::string& known_name : known) {
        list += list.empty() ? "@" : ", @";
        list += known_name;
    }
    return std::format("unknown alias '@{}' (known aliases: {})", name, list);
}

}  // namespace

ScriptResolver::ScriptResolver(std::filesystem::path scriptsRoot, const std::map<std::string, std::string>& aliases)
    : _root{std::move(scriptsRoot)} {
    for (const auto& [name, target] : aliases) {
        if (!ScriptPath::isValidAliasName(name)) {
            throw exceptions::ScriptLoadException{
                std::format("scripts alias '{}': the name must be made of a-z, 0-9, '_', '.' and '-'", name)};
        }
        const std::string key = lowered(name);
        if (key == kScripts || key == kSelf) {
            throw exceptions::ScriptLoadException{std::format("scripts alias '{}': '{}' is reserved", name, key)};
        }
        auto segments = targetSegments(_root, target);
        if (!segments) {
            throw exceptions::ScriptLoadException{
                std::format("scripts alias '{}': the target '{}' must be a folder inside scripts/", name, target)};
        }
        if (!_aliases.emplace(key, *std::move(segments)).second) {
            throw exceptions::ScriptLoadException{
                std::format("scripts alias '{}': defined twice (names ignore case)", name)};
        }
    }
}

const std::filesystem::path& ScriptResolver::root() const noexcept { return _root; }

Result<luau::ModuleSource> ScriptResolver::operator()(const std::string_view requesterKey,
                                                      const std::string_view path) const {
    const Result<ScriptPath> parsed = ScriptPath::parse(path);
    if (!parsed) {
        return tl::unexpected{parsed.error()};
    }
    const Requester requester = splitRequester(requesterKey);
    Segments target;
    if (parsed->kind == ScriptPath::Kind::kRelative) {
        // From an init.luau, "./" names the siblings of its folder: the folder is the module.
        target = requester.folder;
        const std::size_t up = parsed->parents + (requester.isInit ? 1 : 0);
        if (up > target.size()) {
            return fail(ErrorKind::kOutsideScripts, "escapes scripts/");
        }
        target.resize(target.size() - up);
    } else if (parsed->alias == kSelf) {
        if (requesterKey.empty()) {
            return fail(ErrorKind::kInvalidPath, "invalid path: '@self' needs a requiring script");
        }
        target = requester.folder;
    } else if (parsed->alias != kScripts) {
        const auto alias = _aliases.find(parsed->alias);
        if (alias == _aliases.end()) {
            return fail(ErrorKind::kUnknownAlias, unknownAlias(parsed->alias, _aliases));
        }
        target = alias->second;
    }
    target.insert(target.end(), parsed->segments.begin(), parsed->segments.end());
    return load(target);
}

Result<luau::ModuleSource> ScriptResolver::resolveEntry(const std::string_view entry) const {
    if (entry.empty()) {
        return fail(ErrorKind::kInvalidPath, "invalid entry: the path is empty");
    }
    if (entry.starts_with('@') || entry.starts_with('.')) {
        return fail(ErrorKind::kInvalidPath,
                    std::format("invalid entry '{}': give a path relative to scripts/, with no prefix", entry));
    }
    Result<luau::ModuleSource> module = (*this)({}, std::format("@scripts/{}", entry));
    if (!module && module.error().kind() == ErrorKind::kInvalidPath) {
        return fail(ErrorKind::kInvalidPath, std::format("invalid entry '{}': {}", entry, module.error().message()));
    }
    return module;
}

Result<luau::ModuleSource> ScriptResolver::load(const Segments& target) const {
    // `target` is the module's path: look for <target>.luau, then <target>/init.luau.
    struct Candidate {
        Segments folders;
        std::string fileName;
        Lookup found;
    };
    std::vector<Candidate> candidates;
    if (!target.empty()) {
        Segments folders{target.begin(), target.end() - 1};
        candidates.push_back({.folders = std::move(folders), .fileName = target.back() + ".luau", .found = {}});
    }
    candidates.push_back({.folders = target, .fileName = std::string{kInit}, .found = {}});
    for (Candidate& candidate : candidates) {
        candidate.found = walk(_root, candidate.folders, candidate.fileName);
    }

    const auto foundCount =
        std::ranges::count_if(candidates, [](const Candidate& c) { return c.found.status == Lookup::Status::kFound; });
    if (foundCount > 1) {
        return fail(ErrorKind::kAmbiguousModule,
                    std::format("ambiguous: both {} and {} exist",
                                displayed(candidates.front().folders) + "/" + candidates.front().fileName,
                                displayed(candidates.back().folders) + "/" + candidates.back().fileName));
    }
    if (foundCount == 0) {
        for (const Candidate& candidate : candidates) {
            if (candidate.found.status == Lookup::Status::kCaseMismatch) {
                return fail(ErrorKind::kCaseMismatch, caseMessage(candidate.found));
            }
        }
        std::string tried;
        for (const Candidate& candidate : candidates) {
            tried += tried.empty() ? "" : " or ";
            tried += displayed(candidate.folders) + "/" + candidate.fileName;
        }
        return fail(ErrorKind::kModuleNotFound, std::format("module not found: {}", tried));
    }

    const Candidate& chosen =
        *std::ranges::find_if(candidates, [](const Candidate& c) { return c.found.status == Lookup::Status::kFound; });
    Segments parts = chosen.folders;
    parts.push_back(chosen.fileName);
    std::filesystem::path file = _root;
    for (const std::string& part : parts) {
        file /= part;
    }
    if (!core::asset::isInsideDirectory(_root, file)) {
        return fail(ErrorKind::kOutsideScripts, "escapes scripts/ (a link points out of it)");
    }
    luau::ModuleSource source;
    source.key = join(parts);
    source.chunkName = std::format("{}/{}", kScripts, source.key);
    try {
        source.source = core::asset::readTextFile(file);
    } catch (const core::asset::exceptions::AssetLoadException& exception) {
        return fail(ErrorKind::kModuleNotFound, std::format("cannot read {}: {}", source.chunkName, exception.what()));
    }
    return source;
}
}  // namespace rtype::scripting::modules
