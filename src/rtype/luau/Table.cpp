/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Table
*/

#include "Table.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace rtype::luau {
Table::Table() = default;
Table::~Table() noexcept = default;
Table::Table(const Table&) noexcept = default;
Table& Table::operator=(const Table&) noexcept = default;
Table::Table(Table&&) noexcept = default;
Table& Table::operator=(Table&&) noexcept = default;

const Value* Table::tryGet(const std::string_view key) const noexcept {
    const auto it = _map.find(key);
    if (it == _map.end()) {
        return nullptr;
    }
    return &it->second;
}

Value* Table::tryGet(const std::string_view key) noexcept {
    const auto it = _map.find(key);
    if (it == _map.end()) {
        return nullptr;
    }
    return &it->second;
}

Value& Table::set(std::string key, Value value) {
    auto [it, _] = _map.insert_or_assign(std::move(key), std::move(value));
    return it->second;
}

std::size_t Table::size() const noexcept { return _map.size(); }

Table::iterator Table::begin() noexcept { return _map.begin(); }

Table::iterator Table::end() noexcept { return _map.end(); }

Table::const_iterator Table::begin() const noexcept { return _map.begin(); }

Table::const_iterator Table::end() const noexcept { return _map.end(); }

Table::const_iterator Table::cbegin() const noexcept { return _map.cbegin(); }

Table::const_iterator Table::cend() const noexcept { return _map.cend(); }
}  // namespace rtype::luau
