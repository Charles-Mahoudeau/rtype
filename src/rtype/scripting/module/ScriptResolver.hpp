/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptResolver
*/

#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/luau/ModuleSource.hpp"
#include "rtype/scripting/Export.hpp"
#include "rtype/scripting/Result.hpp"

namespace rtype::scripting {
/// @brief Finds the file a `require` path names, inside a game's `scripts/` folder and nowhere else.
///
/// @details Usable as the `luau::ModuleLoader::Resolver` (`operator()`). It parses the path with ScriptPath, resolves
/// it from the requiring file or from an alias, and looks for `<path>.luau` then `<path>/init.luau` (both existing is
/// an error). Three rules make it safe and the same on every system:
///   - nothing outside `scripts/` is reachable: not by `..`, not by a link pointing out (checked on the real path);
///   - the case of every segment must be the one on disk (a mismatch is an error naming the real name), even on a
///     case-insensitive file system;
///   - `/` is the only separator.
///
/// The requester is identified by its key, the path relative to `scripts/` with its extension
/// (`config/player.luau`, `ui/init.luau`). An empty requester is a root require (the entry points of a game): there,
/// `./x` is relative to `scripts/` itself. From an `init.luau`, `./` means the siblings of its folder and `@self` its
/// own folder; from another file `@self` is the file's folder, like `./`. `@scripts` is `scripts/` (alone, it is
/// `scripts/init.luau`).
///
/// See `docs/scripting/plan.md` (D10). Methods are const and thread-safe as far as the file system is.
class RTYPE_SCRIPTING_API ScriptResolver {
  public:
    /// @brief Creates a resolver.
    /// @param scriptsRoot The game's `scripts/` folder.
    /// @param aliases Extra aliases: name, then the target folder relative to `scripts/`. Names are stored lowercase.
    /// @throws exceptions::ScriptLoadException If a name is invalid, is `scripts` or `self`, appears twice (ignoring
    /// case), or if a target is absolute or leaves `scripts/` (by `..` or a link).
    explicit ScriptResolver(std::filesystem::path scriptsRoot, const std::map<std::string, std::string>& aliases = {});

    /// @brief Resolves a require path.
    /// @param requesterKey Key of the requiring file, empty for a root require.
    /// @param path The string given to `require`.
    /// @return The module (key relative to `scripts/` with its extension, chunk name `scripts/<key>`, content), or an
    /// error: `kInvalidPath`, `kUnknownAlias`, `kOutsideScripts`, `kModuleNotFound`, `kAmbiguousModule`,
    /// `kCaseMismatch`.
    [[nodiscard]] Result<luau::ModuleSource> operator()(std::string_view requesterKey, std::string_view path) const;

    /// @brief Resolves an entry point: a path relative to `scripts/`, with no prefix and no extension
    /// (`"server/main"`), exactly like `@scripts/<entry>`.
    /// @return The module, or `kInvalidPath` for a prefix, an extension, an empty, `.` or `..` segment, or what
    /// operator() reports.
    [[nodiscard]] Result<luau::ModuleSource> resolveEntry(std::string_view entry) const;

    /// @return The scripts folder.
    [[nodiscard]] const std::filesystem::path& root() const noexcept;

  private:
    /// @brief Reads the file candidates of a resolved folder path.
    [[nodiscard]] Result<luau::ModuleSource> load(const std::vector<std::string>& target) const;

    std::filesystem::path _root;                               ///< The scripts folder.
    std::map<std::string, std::vector<std::string>> _aliases;  ///< Lowercase name -> target segments.
};
}  // namespace rtype::scripting
