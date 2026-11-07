/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SceneEdit
*/

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "editor/Export.hpp"
#include "plugins/capsule/Capsule.hpp"

namespace rtype::ecs {
class Components;
class World;
}  // namespace rtype::ecs

/// @brief What the editor does to a scene: a capsule (a `.scene`, an `.actor`, a `.capsule`), whose nodes it changes,
/// shows in the world and writes back to its file. No ImGui here: the panels call these.
namespace rtype::editor::scene {

/// @brief A change of the tree of a capsule that copyCapsule() applies.
struct TreeEdit {
    std::optional<std::uint32_t> removed;  ///< A node to leave out, with its subtree; never the root.
    std::optional<std::pair<std::uint32_t, std::uint32_t>> moved;  ///< A node and its new parent: it goes last there.
};

/// @return Whether @p name can name a node: not empty, without '/', neither "." nor "..".
[[nodiscard]] RTYPE_EDITOR_API bool isValidNodeName(std::string_view name) noexcept;

/// @return The children of @p node, in their order.
[[nodiscard]] RTYPE_EDITOR_API std::vector<std::uint32_t> childrenOf(const plugins::capsule::Capsule& capsule,
                                                                     std::uint32_t node);

/// @return Whether @p node is @p ancestor or one of its descendants.
[[nodiscard]] RTYPE_EDITOR_API bool isInSubtree(const plugins::capsule::Capsule& capsule, std::uint32_t node,
                                                std::uint32_t ancestor);

/// @return @p base if no child of @p parent has it, else the first of `base2`, `base3`... none has. A name that is not
/// valid gives "Node".
[[nodiscard]] RTYPE_EDITOR_API std::string uniqueChildName(const plugins::capsule::Capsule& capsule,
                                                           std::uint32_t parent, std::string_view base);

/// @return A copy of @p capsule, at the same path, with @p edit applied: the nodes are numbered again, each parent
/// before its children, and a moved node whose name its new siblings have gets another one (uniqueChildName()).
/// @throws plugins::capsule::exceptions::CapsuleException When @p edit removes the root, or moves a node into its own
/// subtree or the root anywhere.
[[nodiscard]] RTYPE_EDITOR_API plugins::capsule::Capsule copyCapsule(const plugins::capsule::Capsule& capsule,
                                                                     const ecs::Components& components,
                                                                     const TreeEdit& edit = {});

/// @brief Adds a child to @p parent, named @p name or, if a sibling has it, a free name like it (uniqueChildName()),
/// that instances @p instance if given.
/// @return Its index.
RTYPE_EDITOR_API std::uint32_t addChild(plugins::capsule::Capsule& capsule, std::uint32_t parent, std::string_view name,
                                        std::optional<plugins::capsule::CapsuleRef> instance = std::nullopt);

/// @brief Renames @p node, not the root.
/// @return False, renaming nothing, if @p name is not valid or a sibling has it.
RTYPE_EDITOR_API bool renameNode(plugins::capsule::Capsule& capsule, std::uint32_t node, std::string_view name);

/// @brief Defines @p capsule in the world's Capsules, replacing the one with its id, and asks the capsule plugin to
/// respawn in the next frame what was spawned from it (ReloadedCapsules).
RTYPE_EDITOR_API void commit(ecs::World& world, plugins::capsule::Capsule capsule);

/// @brief Asks the capsule plugin to respawn in the next frame what was spawned from the capsule @p id, as it is now.
RTYPE_EDITOR_API void respawn(ecs::World& world, plugins::capsule::CapsuleId id);

/// @brief Shows the capsule @p id in the world, as the level being played (loadCapsule()): the one shown before goes.
/// @return Why it could not be spawned; nothing when it was.
RTYPE_EDITOR_API std::optional<std::string> open(ecs::World& world, plugins::capsule::CapsuleId id);

/// @brief Writes the capsule @p id to its file, under @p assets (format::writeCapsule()).
/// @return Why it could not be written; nothing when it was.
RTYPE_EDITOR_API std::optional<std::string> save(const ecs::World& world, plugins::capsule::CapsuleId id,
                                                 const std::filesystem::path& assets);

/// @brief Creates the capsule file @p path (in the assets, with a capsule file's extension), with a root only, and
/// reads it into the world's Capsules.
/// @return Why it could not be made: a bad extension, a file that exists, a folder that cannot be made.
RTYPE_EDITOR_API std::optional<std::string> createCapsuleFile(ecs::World& world, const std::filesystem::path& assets,
                                                              std::string_view path);

/// @brief Reads the capsule files @p paths names (in the assets) again into the world's Capsules (CapsuleFiles).
/// @return The errors of the files that could not be read.
RTYPE_EDITOR_API std::vector<std::string> readFiles(ecs::World& world, std::span<const std::string> paths);

}  // namespace rtype::editor::scene
