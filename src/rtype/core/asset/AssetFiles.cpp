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

bool isInsideDirectory(const std::filesystem::path& directory, const std::filesystem::path& path) noexcept {
    try {
        std::filesystem::path lexicalDirectory = directory.lexically_normal();
        if (lexicalDirectory.filename().empty() && lexicalDirectory.has_relative_path()) {
            lexicalDirectory = lexicalDirectory.parent_path();  // "dir/" is "dir"
        }
        if (std::ranges::mismatch(lexicalDirectory, path.lexically_normal()).in1 != lexicalDirectory.end()) {
            return false;
        }
        std::error_code error;
        const std::filesystem::path realDirectory = std::filesystem::weakly_canonical(directory, error);
        if (error) {
            return false;
        }
        const std::filesystem::path realPath = std::filesystem::weakly_canonical(path, error);
        if (error) {
            return false;
        }
        return std::ranges::mismatch(realDirectory, realPath).in1 == realDirectory.end();
    } catch (...) {
        return false;
    }
}

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
    if (!isInsideDirectory(directory, file)) {
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
