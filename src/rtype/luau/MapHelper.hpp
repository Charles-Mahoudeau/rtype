/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MapHelper
*/

#pragma once

#include <cstddef>
#include <string_view>

struct StringHash {
    using is_transparent = void;

    [[nodiscard]] std::size_t operator()(const std::string_view str) const noexcept {
        return std::hash<std::string_view>{}(str);
    }
};
