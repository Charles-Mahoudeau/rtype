/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ModuleSource
*/

#pragma once

#include <string>

namespace rtype::luau {
/// @brief What a resolver gives the `ModuleLoader` for a require path: the identity and the code of a module.
struct ModuleSource {
    std::string key;        ///< Identity of the module: two paths with the same key share one instance.
    std::string chunkName;  ///< Name shown in error messages (`<chunkName>:<line>:`); also the script's name.
    std::string source;     ///< Luau source code of the module.
};
}  // namespace rtype::luau
