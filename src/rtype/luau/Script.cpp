/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Script
*/

#include "Script.hpp"

#include <string>
#include <string_view>
#include <utility>

#include "Bytecode.hpp"
#include "Ref.hpp"

namespace rtype::luau {
Script::Script(Ref closure, std::string name, Bytecode bytecode)
    : _closure{std::move(closure)}, _name{std::move(name)}, _bytecode{std::move(bytecode)} {}

const Ref& Script::closure() const { return _closure; }

std::string_view Script::name() const { return _name; }

const Bytecode& Script::bytecode() const { return _bytecode; }
}  // namespace rtype::luau
