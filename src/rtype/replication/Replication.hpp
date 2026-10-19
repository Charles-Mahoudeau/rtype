/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Replication
*/

#pragma once

#include <string_view>

#include "rtype/replication/Export.hpp"

namespace rtype::replication {

/// @brief Name of the library, also the first exported symbol of rtype-replication (a shared library needs one
/// to be linkable on Windows).
[[nodiscard]] RTYPE_REPLICATION_API std::string_view libraryName() noexcept;

}  // namespace rtype::replication
