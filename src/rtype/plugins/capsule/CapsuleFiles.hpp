/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleFiles
*/

#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <span>
#include <string>
#include <vector>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::ecs {
class Components;
}  // namespace rtype::ecs

namespace rtype::plugins::capsule {
class Capsules;

/// @brief The capsule files of a game: the `.capsule`, `.scene` and `.actor` files under its assets, as the resource
/// "rtype.CapsuleFiles" the CapsulePlugin inserts when the game has an asset directory.
/// @details loadAll() reads them all once every plugin is built (CapsulePlugin::finish()), so a file can use the
/// components scripts declare; a file instancing another reads it first. With hot reload, the CapsuleReloadable hands
/// the files saved since to reload(), which reads them again with the files that instance them (their overrides start
/// from its values). A file that fails to read keeps its old version.
class RTYPE_PLUGIN_CAPSULE_API CapsuleFiles {
  public:
    /// @brief What reload() read.
    struct Reread {
        std::vector<CapsuleId> capsules;  ///< The capsules read again, the files that instance them included.
        std::vector<std::string> errors;  ///< Why each file that kept its old version could not be read.
    };

    /// @param assetDirectory The game's assets; capsule paths are relative to it.
    /// @param components The world's components, which the files name; they must outlive this resource.
    CapsuleFiles(std::filesystem::path assetDirectory, const ecs::Components& components);
    ~CapsuleFiles() = default;
    CapsuleFiles(const CapsuleFiles& other) = delete;
    CapsuleFiles& operator=(const CapsuleFiles& other) = delete;
    CapsuleFiles(CapsuleFiles&& other) noexcept = default;
    CapsuleFiles& operator=(CapsuleFiles&& other) noexcept = default;

    /// @brief Reads every capsule file under the assets into `capsules` (Capsules::define()).
    /// @return The capsules read.
    /// @throws exceptions::CapsuleException Naming the file, line and column of what cannot be read.
    std::vector<CapsuleId> loadAll(Capsules& capsules);

    /// @brief Reads again the files `paths` names, with the files that instance them at any depth; a path that is not
    /// a capsule file is skipped. A file that fails to read keeps its old version.
    /// @param paths Files saved or added, by their path in the assets ("actors/bydo.actor").
    /// @return The capsules read again, and the errors of the files that kept their old version.
    Reread reload(Capsules& capsules, std::span<const std::string> paths);

  private:
    /// @return The capsule files under the assets now, by their path in it, sorted.
    [[nodiscard]] std::vector<std::string> scan() const;

    /// @brief Reads one file into `capsules`, after the files it instances that are in `pending`.
    /// @param pending The files still to read in this pass; each file read is taken out of it.
    /// @param read Gets the id of each capsule read: this file, and those it read first.
    void read(Capsules& capsules, const std::string& path, std::set<std::string>& pending,
              std::vector<CapsuleId>& read);

    std::filesystem::path _directory;                    ///< The game's assets.
    const ecs::Components* _components;                  ///< What the files name.
    std::map<std::string, std::set<std::string>> _uses;  ///< The capsules each file instances, by its path.
};

RTYPE_ECS_RESOURCE(CapsuleFiles, "rtype.CapsuleFiles");
}  // namespace rtype::plugins::capsule
