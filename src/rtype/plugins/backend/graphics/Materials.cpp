/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Materials
*/

#include "plugins/backend/graphics/Materials.hpp"

#include <cstddef>

#include "core/asset/AssetId.hpp"

namespace rtype::plugins::backend::graphics {

std::size_t Materials::getCount() const noexcept { return _entries.size(); }

const Materials::Entry* Materials::find(core::asset::AssetId id) const {
    const auto found = _entries.find(id);
    return found != _entries.end() ? &found->second : nullptr;
}

void Materials::set(core::asset::AssetId id, Entry entry) { _entries.insert_or_assign(id, entry); }

void Materials::remove(core::asset::AssetId id) { _entries.erase(id); }

}  // namespace rtype::plugins::backend::graphics
