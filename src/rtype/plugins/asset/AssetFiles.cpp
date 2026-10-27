/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetFiles
*/

#include "plugins/asset/AssetFiles.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>

#include "plugins/backend/exceptions/AssetExceptions.hpp"

namespace rtype::plugins::asset {

std::string normalizePath(const std::filesystem::path& path) { return path.lexically_normal().generic_string(); }

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        throw backend::exceptions::AssetLoadException{std::format("cannot open '{}'", path.generic_string())};
    }
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

}  // namespace rtype::plugins::asset
