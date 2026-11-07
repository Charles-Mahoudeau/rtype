/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptsDirectory
*/

#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace rtype::tests {
/// @brief A scripts directory in the temporary directory, removed with it.
class ScriptsDirectory {
  public:
    explicit ScriptsDirectory(const std::string_view name)
        : _path{std::filesystem::temp_directory_path() / std::string{name}} {
        std::filesystem::remove_all(_path);
        std::filesystem::create_directories(_path);
    }

    ~ScriptsDirectory() {
        std::error_code error;
        std::filesystem::remove_all(_path, error);
    }

    ScriptsDirectory(const ScriptsDirectory& other) = delete;
    ScriptsDirectory& operator=(const ScriptsDirectory& other) = delete;
    ScriptsDirectory(ScriptsDirectory&& other) = delete;
    ScriptsDirectory& operator=(ScriptsDirectory&& other) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return _path; }

    /// @brief Writes a file under the directory, creating its folders.
    void write(const std::string& relative, const std::string_view content) const {
        const auto file = _path / relative;
        std::filesystem::create_directories(file.parent_path());
        std::ofstream{file} << content;
    }

    /// @brief Makes a symbolic link under the directory, creating its folders.
    /// @return False if the system refuses (Windows without the privilege).
    [[nodiscard]] bool link(const std::string& relative, const std::filesystem::path& target) const {
        const auto file = _path / relative;
        std::filesystem::create_directories(file.parent_path());
        std::error_code error;
        if (std::filesystem::is_directory(target)) {
            std::filesystem::create_directory_symlink(target, file, error);  // Windows tells the two kinds apart
        } else {
            std::filesystem::create_symlink(target, file, error);
        }
        return !error;
    }

  private:
    std::filesystem::path _path;  ///< The directory.
};
}  // namespace rtype::tests
