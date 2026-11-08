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

/// @brief Starts text in @p tone; ends with popTextTone().
RTYPE_EDITOR_API void pushTextTone(Tone tone);

/// @brief Ends what pushTextTone() started.
RTYPE_EDITOR_API void popTextTone();

/// @brief Shows @p text in @p tone, without a format string.
RTYPE_EDITOR_API void textTone(const std::string& text, Tone tone);

/// @brief A button filled with @p tone (green Play, red Stop...), the label dark on top of it.
/// @return Whether it was pressed.
RTYPE_EDITOR_API bool toneButton(const std::string& label, Tone tone);

/// @brief A pill with @p text in @p tone, as tall as a button so it sits in a toolbar.
RTYPE_EDITOR_API void chip(const std::string& text, Tone tone);

}  // namespace rtype::editor::gui
