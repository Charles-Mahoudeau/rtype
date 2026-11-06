/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FormsPrelude
*/

#pragma once

#include <string_view>

namespace rtype::scripting::setup {
/// @brief Name of the prelude in error messages.
inline constexpr std::string_view kFormsPreludeName = "engine.forms";

/// @return The Luau source of the prelude: it creates the empty `engine.assets` and `engine.input`, which the plugins
/// fill with their setup forms (ecs::PluginRegistry::addScriptForms()), and `engine.forms`, what those forms read their
/// tables with. Every error starts with the form, and the part of it, that is wrong:
/// `engine.assets.atlas: frame 2: 'size' must be a table {x, y}`.
[[nodiscard]] std::string_view getFormsPrelude() noexcept;
}  // namespace rtype::scripting::setup
