/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetFiles
*/

#include "core/asset/AssetFiles.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>

#include "core/asset/exceptions/AssetExceptions.hpp"

namespace rtype::core::asset {

std::string normalizePath(const std::filesystem::path& path) { return path.lexically_normal().generic_string(); }

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        throw exceptions::AssetLoadException{std::format("cannot open '{}'", path.generic_string())};
    }
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

}  // namespace rtype::core::asset
