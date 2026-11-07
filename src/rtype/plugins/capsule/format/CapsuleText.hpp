/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleText
*/

#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Export.hpp"

namespace rtype::ecs {
class Components;
}  // namespace rtype::ecs

namespace rtype::plugins::capsule {
class Capsules;
}  // namespace rtype::plugins::capsule

/// @brief The capsule file format: a flat list of node sections, each with its components written by field name.
/// @details
/// @code{.unparsed}
/// [capsule format=1]
///
/// [node path="."]
/// game.Health = { value = 50 }
///
/// [node path="LeftTurret" instance="actors/turret.actor"]
/// rtype.Transform = { position = (-40, 0) }
///
/// [node path="Background" side=client]
/// rtype.SpriteRenderer = { texture = "sprites/bg.png", layer = -10 }
/// @endcode
/// The root (`path="."`) comes first, a parent before its children; a node's path names its parent. Values: numbers
/// (`0x` for an asset id with no path), `true` / `false`, `"text"` (strings, text, asset paths), `(x, y)` vectors, and
/// `[a, b]` for arrays and lists. `#` starts a comment. Entity fields cannot be set yet.
namespace rtype::plugins::capsule::format {
/// @brief The extensions of capsule files: the same format, the name only tells a developer what it is for.
inline constexpr std::array<std::string_view, 3> kExtensions{".capsule", ".scene", ".actor"};

/// @brief The version of the format the writer writes and the reader reads.
inline constexpr int kFormatVersion = 1;

/// @brief Called with the path of each capsule a node instances, before the reader looks it up: the loader reads that
/// file first, so the node's fields start from the instanced root's values.
using InstanceLoader = std::function<void(std::string_view path)>;

/// @return Whether `path` has a capsule file's extension.
[[nodiscard]] RTYPE_PLUGIN_CAPSULE_API bool isCapsuleFile(const std::filesystem::path& path);

/// @brief Reads the text of a capsule file into a capsule, without defining it: Capsules::define() then replaces the
/// old version in one step, so a file that fails to read leaves it as it was.
/// @details A component's value starts from its defaults, or, on a node that instances another capsule, from that
/// capsule root's value for the component (an override names only the fields it changes). The asset paths read are
/// remembered in `capsules`, for writeCapsule().
/// @param path The capsule's path, normalized: its id.
/// @param components The world's components: the names a file uses.
/// @param loadInstance Called before an instanced capsule is looked up; may be empty.
/// @throws exceptions::CapsuleException Naming the file, the line and the column of what cannot be read.
[[nodiscard]] RTYPE_PLUGIN_CAPSULE_API Capsule readCapsule(std::string_view path, std::string_view text,
                                                           const ecs::Components& components, Capsules& capsules,
                                                           const InstanceLoader& loadInstance = {});

/// @brief Writes a capsule as the text of its file: each node, and each of its components with the fields that differ
/// from where reading starts (the defaults, or an instanced root's value), so reading the text back gives the same
/// capsule.
/// @details An asset id is written as the path it was read from (Capsules::findAssetPath()), or as its id. Entity
/// fields are not written.
[[nodiscard]] RTYPE_PLUGIN_CAPSULE_API std::string writeCapsule(const Capsule& capsule,
                                                                const ecs::Components& components,
                                                                const Capsules& capsules);
}  // namespace rtype::plugins::capsule::format
