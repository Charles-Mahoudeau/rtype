/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Script
*/

#include "Script.hpp"

#include <lua.h>

#include <string>
#include <string_view>
#include <utility>

#include "Bytecode.hpp"
#include "Ref.hpp"

namespace rtype::luau {
Script::Script(lua_State* state, Ref thread, Ref closure, std::string name)
    : _state{state}, _thread{std::move(thread)}, _closure{std::move(closure)}, _name{std::move(name)} {}

lua_State* Script::state() const { return _state; }

const Ref& Script::thread() const { return _thread; }

const Ref& Script::closure() const { return _closure; }

std::string_view Script::name() const { return _name; }
}  // namespace rtype::luau
