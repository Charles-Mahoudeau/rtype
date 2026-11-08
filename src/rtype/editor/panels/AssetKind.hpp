/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetKind
*/

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "editor/Export.hpp"

namespace rtype::editor::panels {

/// @brief The kinds of entries the Assets panel tells apart, each with its own icon and color.
enum class AssetKind : std::uint8_t {
    kFolder,
    kScene,    ///< A `.scene` capsule.
    kActor,    ///< An `.actor` capsule.
    kCapsule,  ///< Any other capsule file.
    kScript,   ///< Luau.
    kImage,
    kAudio,
    kFont,
    kData,  ///< Structured data: JSON, tilemaps.
    kText,
    kOther,
};

/// @return The kind of the entry named @p name (a file name or a path), from its extension; a folder is told by
/// @p isFolder, not by its name.
[[nodiscard]] RTYPE_EDITOR_API AssetKind kindOf(std::string_view name, bool isFolder);

/// @return The glyph (a code point of the Nerd Font the editor embeds) that is the icon of @p kind; the folder's is the
/// open one when @p open.
[[nodiscard]] RTYPE_EDITOR_API char32_t iconOf(AssetKind kind, bool open = false);

/// @return The name of @p kind, as shown in a tooltip.
[[nodiscard]] RTYPE_EDITOR_API std::string_view labelOf(AssetKind kind);

/// @return The color of the icon of @p kind, as 0xRRGGBB.
[[nodiscard]] RTYPE_EDITOR_API std::uint32_t colorOf(AssetKind kind);

/// @return @p codePoint encoded as UTF-8, to hand to ImGui as text.
[[nodiscard]] RTYPE_EDITOR_API std::string toUtf8(char32_t codePoint);

}  // namespace rtype::editor::panels
