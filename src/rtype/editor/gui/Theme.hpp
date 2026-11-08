/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Theme
*/

#pragma once

#include <cstdint>
#include <string>

#include "editor/Export.hpp"

namespace rtype::editor::gui {

/// @brief What a color says. Panels name a tone, never a color: the theme decides how it looks.
enum class Tone : std::uint8_t {
    kText,     ///< Plain text.
    kMuted,    ///< Secondary information.
    kAccent,   ///< The editor's own color: what is selected or focused.
    kSuccess,  ///< Running, saved, healthy.
    kWarning,  ///< Paused, starting, not saved.
    kDanger,   ///< Stop, error, failure.
    kInfo,     ///< Neutral action or information.
    kOnline,   ///< Network: a server, a client online.
};

/// @brief Applies the editor's look to the current Dear ImGui context: a Catppuccin Mocha palette in three layers of
/// depth (recessed fields and gaps, panels, bars), one accent color with tinted states, rounded shapes and room to
/// breathe.
RTYPE_EDITOR_API void applyTheme();

/// @brief Starts the toolbar: a bar under the menu bar, as wide as the window, padded all round, in which items are
/// laid out with SameLine() and sit centered between its top and bottom. Always call endToolbar(), even when this
/// returns false.
/// @param id The window's name, unique.
RTYPE_EDITOR_API bool beginToolbar(const char* id);

/// @brief Ends what beginToolbar() started.
RTYPE_EDITOR_API void endToolbar();

/// @brief Starts a group of the toolbar, placed on the line of the previous one at @p alignment of the toolbar's
/// width: 0 is the start (the first group), 0.5 the center, 1 the end. It uses the width the group had the frame
/// before (kept in the toolbar's own storage), and never overlaps the previous group. Pair it with endToolbarGroup().
/// @param id The group's name, unique in the toolbar.
RTYPE_EDITOR_API void beginToolbarGroup(const char* id, float alignment);

/// @brief Ends what beginToolbarGroup() started, and keeps the group's width for the next frame.
RTYPE_EDITOR_API void endToolbarGroup();

/// @brief A button with no fill until it is hovered: a menu of the toolbar.
/// @return Whether it was pressed.
RTYPE_EDITOR_API bool flatButton(const std::string& label);

/// @brief Compact metrics for the rows of a tree (padding and indent), until popTreeMetrics(). The roomy padding of
/// the theme widens the arrow every row reserves, so the labels of a tree drift away from their parents.
RTYPE_EDITOR_API void pushTreeMetrics();

/// @brief Ends what pushTreeMetrics() started.
RTYPE_EDITOR_API void popTreeMetrics();

/// @brief Starts text in @p tone; ends with popTextTone().
RTYPE_EDITOR_API void pushTextTone(Tone tone);

/// @brief Ends what pushTextTone() started.
RTYPE_EDITOR_API void popTextTone();

/// @brief Draws with the heavier font until popEmphasisFont(): for text on a filled shape, where the regular weight
/// looks faded. The heavier font is the last one the atlas loaded.
RTYPE_EDITOR_API void pushEmphasisFont();

/// @brief Ends what pushEmphasisFont() started.
RTYPE_EDITOR_API void popEmphasisFont();

/// @brief Shows @p text in @p tone, without a format string.
RTYPE_EDITOR_API void textTone(const std::string& text, Tone tone);

/// @brief A button tinted with @p tone (green Play, red Stop...), its label and outline in that color.
/// @return Whether it was pressed.
RTYPE_EDITOR_API bool toneButton(const std::string& label, Tone tone);

/// @brief A pill with @p text in @p tone, as tall as a button so it sits in a toolbar.
RTYPE_EDITOR_API void chip(const std::string& text, Tone tone);

}  // namespace rtype::editor::gui
