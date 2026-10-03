/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Settings
*/

#include "Settings.hpp"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "engine/exceptions/ConfigExceptions.hpp"

namespace rtype::engine::config {

namespace {

/// @return The value of the key as a T, the fallback if the key has none.
/// @throws exceptions::SettingsException If the value is not a T.
template <typename T>
T read(const std::map<std::string, Settings::Value, std::less<>>& values, std::string_view key, T fallback,
       std::string_view expected) {
    const auto it = values.find(key);
    if (it == values.end()) {
        return fallback;
    }
    if (const T* value = std::get_if<T>(&it->second)) {
        return *value;
    }
    throw exceptions::SettingsException("Setting '" + std::string(key) + "' must be " + std::string(expected));
}

}  // namespace

void Settings::set(std::string key, Value value) { _values.insert_or_assign(std::move(key), std::move(value)); }

bool Settings::has(std::string_view key) const { return _values.contains(key); }

Settings Settings::section(std::string_view prefix) const {
    const std::string start = std::string(prefix) + ".";
    Settings result;
    for (const auto& [key, value] : _values) {
        if (key.starts_with(start)) {
            result.set(key.substr(start.size()), value);
        }
    }
    return result;
}

bool Settings::getBool(std::string_view key, bool fallback) const {
    return read<bool>(_values, key, fallback, "a boolean");
}

double Settings::getNumber(std::string_view key, double fallback) const {
    return read<double>(_values, key, fallback, "a number");
}

std::string Settings::getString(std::string_view key, std::string_view fallback) const {
    return read<std::string>(_values, key, std::string(fallback), "a string");
}

std::vector<double> Settings::getNumberList(std::string_view key, std::vector<double> fallback) const {
    if (const auto it = _values.find(key); it != _values.end()) {
        if (const auto* strings = std::get_if<std::vector<std::string>>(&it->second);
            strings != nullptr && strings->empty()) {
            return {};
        }
    }
    return read<std::vector<double>>(_values, key, std::move(fallback), "a list of numbers");
}

std::vector<std::string> Settings::getStringList(std::string_view key, std::vector<std::string> fallback) const {
    if (const auto it = _values.find(key); it != _values.end()) {
        if (const auto* numbers = std::get_if<std::vector<double>>(&it->second);
            numbers != nullptr && numbers->empty()) {
            return {};
        }
    }
    return read<std::vector<std::string>>(_values, key, std::move(fallback), "a list of strings");
}

void Settings::checkKeys(std::initializer_list<std::string_view> allowed, std::string_view context) const {
    for (const auto& [key, value] : _values) {
        const bool known = std::ranges::any_of(allowed, [&key](std::string_view name) {
            return key == name || (key.starts_with(name) && key.size() > name.size() && key.at(name.size()) == '.');
        });
        if (!known) {
            throw exceptions::SettingsException("Unknown setting '" + key + "' in '" + std::string(context) + "'");
        }
    }
}

}  // namespace rtype::engine::config
