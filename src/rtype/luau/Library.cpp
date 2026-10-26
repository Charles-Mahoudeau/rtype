/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include "Library.hpp"

#include <deque>

namespace rtype::luau {
Library::Library(std::string name, Table table, std::deque<Member> ownedMembers)
    : _name{std::move(name)}, _table{std::move(table)}, _ownedMembers{std::move(ownedMembers)} {}

std::string_view Library::name() const { return _name; }

const Table& Library::table() const { return _table; }
}  // namespace rtype::luau
