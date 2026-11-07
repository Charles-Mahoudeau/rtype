/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptProbe
*/

#pragma once

#include <string_view>

namespace rtype::tests {
/// @brief Setup forms (see ecs::PluginRegistry::addScriptForms()) giving the scripts of a test a `probe` global, a
/// table whose writes outlive the script: the scripts of a host have globals of their own, so a test that has a
/// system record what it saw, and reads it in a later `run`, goes through it. Add them before the first script runs.
inline constexpr std::string_view kProbeForms = R"(
    local store = {}
    probe = setmetatable({}, {
        __index = function(_, key) return store[key] end,
        __newindex = function(_, key, value) store[key] = value end,
    })
)";
}  // namespace rtype::tests
