/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LibraryBuilder
*/

#include "LibraryBuilder.hpp"

#include <cassert>
#include <format>
#include <string>
#include <utility>

#include "Function.hpp"
#include "Library.hpp"
#include "Runtime.hpp"
#include "Table.hpp"
#include "Value.hpp"

namespace rtype::luau {
namespace {
/// Creates the table of a library.
/// @throws exceptions::RuntimeError If the table cannot be created.
Table createLibraryTable(const Runtime& runtime) {
    auto result = Table::create(runtime.state());
    if (!result) {
        throw exceptions::RuntimeError{std::format("cannot create the library table: {}", result.error().message())};
    }
    return *std::move(result);
}
}  // namespace

LibraryBuilder::LibraryBuilder(Runtime& runtime) : _runtime{runtime}, _table{createLibraryTable(runtime)} {}

Library LibraryBuilder::build() { return Library{std::move(_name), std::move(_table), std::move(_ownedMembers)}; }

void LibraryBuilder::setName(const std::string& name) {
    _name = name;
    [[maybe_unused]] const auto result = _table.set("__name", {_name});
    assert(result);
}

void LibraryBuilder::registryLibrary(Library library) {
    const auto& it = _ownedMembers.emplace_back(std::move(library));
    const auto& lib = std::get<Library>(it);
    const auto result = table().registerLibrary(lib);
    assert(result);
}

Function& LibraryBuilder::function(std::string name) {
    auto& it = _ownedMembers.emplace_back(Function::create(_runtime.state(), name));
    auto& function = std::get<Function>(it);
    const auto result = table().set(Value{std::move(name)}, Value{function.ref()});
    assert(result);
    return function;
}

Table& LibraryBuilder::table() { return _table; }
}  // namespace rtype::luau
