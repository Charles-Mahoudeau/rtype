/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Assets
*/

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace rtype::plugins::backend {
template <typename Id>
std::size_t Assets<Id>::getCount() const noexcept {
    return _entries.size();
}

template <typename Id>
bool Assets<Id>::add(std::string name, Id id, Info info) {
    if (findEntry(std::string_view{name}) != nullptr || findEntry(id) != nullptr) {
        return false;
    }
    _entries.push_back(Entry{.name = std::move(name), .id = id, .info = std::move(info)});
    return true;
}

template <typename Id>
std::optional<Id> Assets<Id>::find(std::string_view name) const noexcept {
    const Entry* entry = findEntry(name);
    return entry != nullptr ? std::optional<Id>{entry->id} : std::nullopt;
}

template <typename Id>
auto Assets<Id>::getInfo(Id id) const noexcept -> const Info* {
    const Entry* entry = findEntry(id);
    return entry != nullptr ? &entry->info : nullptr;
}

template <typename Id>
std::optional<std::string_view> Assets<Id>::getName(Id id) const noexcept {
    const Entry* entry = findEntry(id);
    return entry != nullptr ? std::optional<std::string_view>{entry->name} : std::nullopt;
}

template <typename Id>
bool Assets<Id>::unload(Owner& owner, std::string_view name) {
    const auto found = std::ranges::find_if(_entries, [name](const Entry& entry) { return entry.name == name; });
    if (found == _entries.end()) {
        return false;
    }
    owner.destroy(found->id);
    _entries.erase(found);
    return true;
}

template <typename Id>
auto Assets<Id>::findEntry(std::string_view name) const noexcept -> const Entry* {
    const auto found = std::ranges::find_if(_entries, [name](const Entry& entry) { return entry.name == name; });
    return found != _entries.end() ? &*found : nullptr;
}

template <typename Id>
auto Assets<Id>::findEntry(Id id) const noexcept -> const Entry* {
    const auto found = std::ranges::find_if(_entries, [id](const Entry& entry) { return entry.id == id; });
    return found != _entries.end() ? &*found : nullptr;
}
}  // namespace rtype::plugins::backend
