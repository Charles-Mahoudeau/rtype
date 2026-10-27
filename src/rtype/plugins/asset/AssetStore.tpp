/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetStore
*/

#include <cstddef>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "plugins/asset/AssetFiles.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/exceptions/AssetExceptions.hpp"

namespace rtype::plugins::asset {

template <typename T>
AssetStore<T>::AssetStore(std::string kind) : _kind{std::move(kind)} {}

template <typename T>
std::size_t AssetStore<T>::getCount() const noexcept {
    return _entries.size();
}

template <typename T>
const T* AssetStore<T>::get(AssetId id) const {
    const Entry* entry = getEntry(id);
    return entry != nullptr && entry->value ? &*entry->value : nullptr;
}

template <typename T>
auto AssetStore<T>::getEntry(AssetId id) const -> const Entry* {
    const auto found = _entries.find(id);
    return found != _entries.end() ? &found->second : nullptr;
}

template <typename T>
template <typename Parse>
    requires std::invocable<const Parse&, std::string_view, const std::filesystem::path&>
auto AssetStore<T>::load(const std::filesystem::path& path, const Parse& parse) -> Stored {
    std::string name = normalizePath(path);
    const std::optional<AssetId> id = reserve(name);
    if (!id) {
        return Stored{.id = AssetId::fromPath(name)};
    }
    std::string text;
    try {
        text = readTextFile(path);
    } catch (const backend::exceptions::AssetLoadException& error) {
        fail(*id, name, std::format("{} '{}': {}", _kind, name, error.what()));
        return Stored{.id = *id};
    }
    try {
        return Stored{.id = *id, .added = &store(*id, name, parse(std::string_view{text}, path))};
    } catch (const std::exception& error) {
        fail(*id, std::move(name), error.what());
    }
    return Stored{.id = *id};
}

template <typename T>
auto AssetStore<T>::add(const std::string& name, T value) -> Stored {
    std::string path = normalizePath(name);
    const std::optional<AssetId> id = reserve(path);
    if (!id) {
        return Stored{.id = AssetId::fromPath(path)};
    }
    return Stored{.id = *id, .added = &store(*id, std::move(path), std::move(value))};
}

template <typename T>
bool AssetStore<T>::unload(AssetId id) {
    return _entries.erase(id) != 0;
}

template <typename T>
std::optional<AssetId> AssetStore<T>::reserve(const std::string& path) const {
    const AssetId id = AssetId::fromPath(path);
    if (const Entry* known = getEntry(id)) {
        if (known->path != path) {
            throw backend::exceptions::AssetLoadException{
                std::format("{} '{}' has the same id as '{}': rename one of them", _kind, path, known->path)};
        }
        return std::nullopt;
    }
    return id;
}

template <typename T>
const T& AssetStore<T>::store(AssetId id, std::string path, T value) {
    Entry entry;
    entry.path = std::move(path);
    entry.value.emplace(std::move(value));
    return *_entries.insert_or_assign(id, std::move(entry)).first->second.value;
}

template <typename T>
void AssetStore<T>::fail(AssetId id, std::string path, std::string error) {
    std::cerr << '[' << _kind << "Assets] " << error << '\n';
    Entry entry;
    entry.path = std::move(path);
    entry.error = std::move(error);
    _entries.insert_or_assign(id, std::move(entry));
}

}  // namespace rtype::plugins::asset
