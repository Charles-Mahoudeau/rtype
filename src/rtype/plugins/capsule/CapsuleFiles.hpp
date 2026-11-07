/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleFiles
*/

#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <set>
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
/// components scripts declare; a file instancing another reads it first. With hot reload, update() compares the files'
/// modification times every kInterval and reads again a file saved since, with the files that instance it (their
/// overrides start from its values), then the plugin reloads what was spawned from them. A file that fails to read
/// then keeps its old version, and the error is written to the error output.
class RTYPE_PLUGIN_CAPSULE_API CapsuleFiles {
  public:
    using Clock = std::chrono::steady_clock;                    ///< What update() is timed with.
    static constexpr std::chrono::milliseconds kInterval{500};  ///< Between two looks at the files.

    /// @param assetDirectory The game's assets; capsule paths are relative to it.
    /// @param components The world's components, which the files name; they must outlive this resource.
    CapsuleFiles(std::filesystem::path assetDirectory, const ecs::Components& components);
    ~CapsuleFiles() = default;
    CapsuleFiles(const CapsuleFiles& other) = delete;
    CapsuleFiles& operator=(const CapsuleFiles& other) = delete;
    CapsuleFiles(CapsuleFiles&& other) noexcept = default;
    CapsuleFiles& operator=(CapsuleFiles&& other) noexcept = default;

    /// @return The game's assets.
    [[nodiscard]] const std::filesystem::path& getDirectory() const noexcept;

    /// @brief Reads every capsule file under the assets into `capsules` (Capsules::define()).
    /// @return The capsules read.
    /// @throws exceptions::CapsuleException Naming the file, line and column of what cannot be read.
    std::vector<CapsuleId> loadAll(Capsules& capsules);

    /// @brief Looks at the files if kInterval went by, and reads again those saved or added since, with the files that
    /// instance them; a file that fails to read keeps its old version (the error goes to the error output).
    /// @return The capsules read again.
    std::vector<CapsuleId> update(Capsules& capsules, Clock::time_point now);

  private:
    /// @brief When each capsule file was last written, by its path in the assets.
    using WriteTimes = std::map<std::string, std::filesystem::file_time_type>;

    /// @return The capsule files under the assets now.
    [[nodiscard]] WriteTimes scan() const;

    /// @brief Reads one file into `capsules`, after the files it instances that are in `pending`.
    /// @param pending The files still to read in this pass; each file read is taken out of it.
    /// @param read Gets the id of each capsule read: this file, and those it read first.
    void read(Capsules& capsules, const std::string& path, std::set<std::string>& pending,
              std::vector<CapsuleId>& read);

    std::filesystem::path _directory;                    ///< The game's assets.
    const ecs::Components* _components;                  ///< What the files name.
    WriteTimes _files;                                   ///< The files at the last look.
    std::map<std::string, std::set<std::string>> _uses;  ///< The capsules each file instances, by its path.
    Clock::time_point _nextPoll;                         ///< When update() looks at the files again.
};

RTYPE_ECS_RESOURCE(CapsuleFiles, "rtype.CapsuleFiles");
}  // namespace rtype::plugins::capsule
