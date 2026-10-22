/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Table
*/

#include "Table.hpp"

#include <lua.h>

#include <cmath>
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ErrorKind.hpp"
#include "Exceptions.hpp"
#include "Failure.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "Value.hpp"

namespace {
/// @brief Stack slots reserved by an operation: the table, a key, a value, and one spare.
constexpr int kReservedSlots = 4;

/// @brief Restores the size of the stack of a Lua state when it goes out of scope.
class StackGuard {
  public:
    explicit StackGuard(lua_State* state) noexcept : _state{state}, _top{lua_gettop(state)} {}
    ~StackGuard() noexcept { lua_settop(_state, _top); }
    StackGuard(const StackGuard&) = delete;
    StackGuard& operator=(const StackGuard&) = delete;
    StackGuard(StackGuard&&) = delete;
    StackGuard& operator=(StackGuard&&) = delete;

  private:
    lua_State* _state;  ///< The Lua state whose stack is restored.
    int _top;           ///< Size of the stack to restore.
};

[[nodiscard]] bool reserveStack(lua_State* state) noexcept { return lua_checkstack(state, kReservedSlots) != 0; }

[[nodiscard]] rtype::luau::Failure stackOverflow() {
    return {rtype::luau::ErrorKind::kStackOverflow, "Lua stack overflow"};
}
}  // namespace

namespace rtype::luau {
Result<Table> Table::create(lua_State* state, const int arrayCapacity, const int hashCapacity) {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    if (lua_checkstack(state, 1) == 0) {
        return stackOverflow();
    }
    lua_createtable(state, arrayCapacity, hashCapacity);
    return Table{Ref::pop(state)};
}

Result<Table> Table::fromStack(lua_State* state, const int stackIndex) {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    if (const int type = lua_type(state, stackIndex); type != LUA_TTABLE) {
        return Failure{ErrorKind::kTypeMismatch, std::format("expected a table, got '{}'", lua_typename(state, type))};
    }
    return Table{Ref{state, stackIndex}};
}

Table::~Table() noexcept = default;
Table::Table(const Table&) noexcept = default;
Table& Table::operator=(const Table&) noexcept = default;
Table::Table(Table&&) noexcept = default;
Table& Table::operator=(Table&&) noexcept = default;

Result<Value> Table::get(const Value& key) const {
    lua_State* const target{state()};
    if (!reserveStack(target)) {
        return stackOverflow();
    }
    const StackGuard guard{target};
    _ref->push(target);
    if (auto pushed = key.push(target); !pushed) {
        return forwardError<Value>(std::move(pushed));
    }
    lua_rawget(target, -2);
    return Value::fromStack(target, Ref::kLuaStackTop);
}

Result<Value> Table::get(const std::string_view key) const { return get(Value{std::string{key}}); }

Result<Value> Table::getAt(const std::size_t index) const { return get(Value{static_cast<double>(index)}); }

Result<void> Table::set(const Value& key, const Value& value) {
    if (key.is<Value::Nil>()) {
        return Failure{ErrorKind::kTypeMismatch, "table index is nil"};
    }
    if (const auto* number = key.tryAs<double>(); number != nullptr && std::isnan(*number)) {
        return Failure{ErrorKind::kTypeMismatch, "table index is NaN"};
    }
    lua_State* const target{state()};
    if (!reserveStack(target)) {
        return stackOverflow();
    }
    const StackGuard guard{target};
    _ref->push(target);
    if (lua_getreadonly(target, -1) != 0) {
        return Failure{ErrorKind::kRuntime, "attempt to modify a readonly table"};
    }
    if (auto pushed = key.push(target); !pushed) {
        return pushed;
    }
    if (auto pushed = value.push(target); !pushed) {
        return pushed;
    }
    lua_rawset(target, -3);
    return {};
}

Result<void> Table::set(const std::string_view key, const Value& value) { return set(Value{std::string{key}}, value); }

Result<void> Table::setAt(const std::size_t index, const Value& value) {
    return set(Value{static_cast<double>(index)}, value);
}

Result<bool> Table::has(const Value& key) const {
    lua_State* const target{state()};
    if (!reserveStack(target)) {
        return stackOverflow();
    }
    const StackGuard guard{target};
    _ref->push(target);
    if (auto pushed = key.push(target); !pushed) {
        return forwardError<bool>(std::move(pushed));
    }
    lua_rawget(target, -2);
    return lua_type(target, -1) != LUA_TNIL;
}

Result<bool> Table::has(const std::string_view key) const { return has(Value{std::string{key}}); }

Result<std::size_t> Table::size() const {
    lua_State* const target{state()};
    if (!reserveStack(target)) {
        return stackOverflow();
    }
    const StackGuard guard{target};
    _ref->push(target);
    return static_cast<std::size_t>(lua_objlen(target, -1));
}

Result<std::size_t> Table::count() const {
    lua_State* const target{state()};
    if (!reserveStack(target)) {
        return stackOverflow();
    }
    const StackGuard guard{target};
    _ref->push(target);
    lua_pushnil(target);
    std::size_t total{};
    while (lua_next(target, -2) != 0) {
        ++total;
        lua_pop(target, 1);
    }
    return total;
}

Result<std::vector<Table::Entry>> Table::entries() const {
    lua_State* const target{state()};
    if (!reserveStack(target)) {
        return stackOverflow();
    }
    const StackGuard guard{target};
    _ref->push(target);
    lua_pushnil(target);
    std::vector<Entry> result{};
    // Each step has the table, the key and the value on the stack. The key stays on it for the next lua_next, and
    // it is only read, never converted in place.
    while (lua_next(target, -2) != 0) {
        auto key = Value::fromStack(target, -2);
        if (!key) {
            return forwardError<std::vector<Entry>>(std::move(key));
        }
        auto value = Value::fromStack(target, -1);
        if (!value) {
            return forwardError<std::vector<Entry>>(std::move(value));
        }
        result.emplace_back(std::move(*key), std::move(*value));
        lua_pop(target, 1);
    }
    return result;
}

Result<Table::Map> Table::toMap() const {
    auto all = entries();
    if (!all) {
        return forwardError<Map>(std::move(all));
    }
    Map result{};
    for (auto& [key, value] : *all) {
        const auto* name = key.tryAs<std::string>();
        if (name == nullptr) {
            return Failure{ErrorKind::kTypeMismatch, "the table has a key that is not a string"};
        }
        result.insert_or_assign(*name, std::move(value));
    }
    return result;
}

Result<void> Table::push(lua_State* state) const {
    if (state == nullptr) {
        throw exceptions::InvalidState{};
    }
    if (!_ref) {
        throw exceptions::InvalidRef{};
    }
    if (lua_mainthread(state) != _ref->state()) {
        return Failure{ErrorKind::kRuntime, "the table belongs to another Lua state"};
    }
    _ref->push(state);
    return {};
}

Result<bool> Table::refersToSame(const Table& other) const {
    lua_State* const target{state()};
    if (target != other.state()) {
        return false;
    }
    if (!reserveStack(target)) {
        return stackOverflow();
    }
    const StackGuard guard{target};
    _ref->push(target);
    other._ref->push(target);
    return lua_rawequal(target, -1, -2) != 0;
}

Table::Table(Ref ref) : _ref{std::make_shared<Ref>(std::move(ref))} {}

lua_State* Table::state() const {
    if (!_ref) {
        throw exceptions::InvalidRef{};
    }
    return _ref->state();
}
}  // namespace rtype::luau
