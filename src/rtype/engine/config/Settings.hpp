/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Settings
*/

#pragma once

#include <initializer_list>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace rtype::engine::config {

/// @brief Configuration values, independent of where they come from (filled in code for now, a config file later).
///
/// @details Flat: nested tables are flattened into dotted keys. The config file
/// @code{.lua}
/// return { renderer = "vulkan", window = { title = "R-Type", size = { 1280, 720 } } }
/// @endcode
/// gives the keys `renderer`, `window.title` and `window.size`. Each part of the engine reads its own section()
/// and turns it into its own typed configuration, checking it with checkKeys() so that a typo is reported instead
/// of silently ignored.
class Settings {
  public:
    using Value = std::variant<bool, double, std::string, std::vector<double>, std::vector<std::string>>;

    Settings() = default;
    ~Settings() = default;
    Settings(const Settings&) = default;
    Settings& operator=(const Settings&) = default;
    Settings(Settings&&) noexcept = default;
    Settings& operator=(Settings&&) noexcept = default;

    /// @brief Sets a value, replacing the previous one.
    void set(std::string key, Value value);

    /// @return True if the key has a value.
    [[nodiscard]] bool has(std::string_view key) const;

    /// @return The keys under `prefix.`, without the prefix: section("window") turns `window.title` into `title`.
    [[nodiscard]] Settings section(std::string_view prefix) const;

    /// @name Typed reads
    /// @brief Return the value of the key, or the fallback if it has none.
    /// @throws exceptions::SettingsException If the key has a value of another type.
    /// @{
    [[nodiscard]] bool getBool(std::string_view key, bool fallback) const;
    [[nodiscard]] double getNumber(std::string_view key, double fallback) const;
    [[nodiscard]] std::string getString(std::string_view key, std::string_view fallback) const;
    [[nodiscard]] std::vector<double> getNumberList(std::string_view key, std::vector<double> fallback) const;
    [[nodiscard]] std::vector<std::string> getStringList(std::string_view key, std::vector<std::string> fallback) const;
    /// @}

    /// @brief Rejects the keys nobody reads: a typo in a config file must not go unnoticed.
    /// @param allowed The keys this section may contain; "name" also allows a whole subsection "name.*".
    /// @param context Name of the section, for the error message.
    /// @throws exceptions::SettingsException Naming the first unknown key.
    void checkKeys(std::initializer_list<std::string_view> allowed, std::string_view context) const;

  private:
    std::map<std::string, Value, std::less<>> _values;  ///< Values by dotted key.
};
}  // namespace rtype::engine::config
