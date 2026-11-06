/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetFiles
*/

#include "core/asset/AssetFiles.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>

#include "core/asset/exceptions/AssetExceptions.hpp"

namespace rtype::core::asset {

std::string normalizePath(const std::filesystem::path& path) { return path.lexically_normal().generic_string(); }

std::filesystem::path resolveAssetPath(const std::filesystem::path& directory, const std::filesystem::path& path) {
    if (directory.empty()) {
        return path;
    }
    const std::string name = normalizePath(path);
    const std::filesystem::path relative{name};
    const std::string_view text{name};
    if (relative.has_root_path() || text.starts_with('/') || text.starts_with('\\') || text == ".." ||
        text.starts_with("../") || text.starts_with("..\\")) {
        throw exceptions::AssetLoadException{std::format("'{}' is outside the assets directory", name)};
    }
    const std::filesystem::path file = directory / relative;
    std::error_code error;
    const std::filesystem::path realDirectory = std::filesystem::weakly_canonical(directory, error);
    const std::filesystem::path realFile = error ? file : std::filesystem::weakly_canonical(file, error);
    if (error || std::ranges::mismatch(realDirectory, realFile).in1 != realDirectory.end()) {
        throw exceptions::AssetLoadException{std::format("'{}' is outside the assets directory", name)};
    }
    return file;
}

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file{path, std::ios::binary};
    if (!file) {
        throw exceptions::AssetLoadException{std::format("cannot open '{}'", path.generic_string())};
    }
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

}  // namespace rtype::core::asset
