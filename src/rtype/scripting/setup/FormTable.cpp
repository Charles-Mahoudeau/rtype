/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FormTable
*/

#include "FormTable.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"

namespace rtype::scripting::setup {
namespace {
constexpr std::array<std::string_view, 2> kVec2Keys{"x", "y"};

/// @return The keys of a table, quoted and separated by commas: `'x', 'y'`.
[[nodiscard]] std::string quoted(const std::span<const std::string_view> keys) {
    std::string text;
    for (const std::string_view key : keys) {
        text += text.empty() ? std::format("'{}'", key) : std::format(", '{}'", key);
    }
    return text;
}

/// @return The entries of @p value.
/// @throws luau::exceptions::RuntimeError If it is not a table, or cannot be read.
[[nodiscard]] std::vector<luau::Table::Entry> entriesOf(const std::string_view context, const luau::Value& value) {
    const auto* table = value.tryAs<luau::Table>();
    if (table == nullptr) {
        throw luau::exceptions::RuntimeError{std::format("{}: expected a table", context)};
    }
    auto entries = table->entries();
    if (!entries) {
        throw luau::exceptions::RuntimeError{std::format("{}: {}", context, entries.error().message())};
    }
    return std::move(*entries);
}
}  // namespace

FormTable FormTable::optional(std::string context, const luau::Value& value,
                              const std::span<const std::string_view> keys) {
    if (value.is<luau::Value::Nil>()) {
        return FormTable{std::move(context)};
    }
    return FormTable{std::move(context), value, keys};
}

FormTable::FormTable(std::string context, const luau::Value& value, const std::span<const std::string_view> keys)
    : _context{std::move(context)} {
    for (auto& [key, item] : entriesOf(_context, value)) {
        const auto* name = key.tryAs<std::string>();
        if (name == nullptr || std::ranges::find(keys, *name) == keys.end()) {
            const std::string given = name != nullptr ? std::format("'{}'", *name) : std::string{"a key"};
            fail(keys.empty() ? std::format("unexpected {}: it takes no keys", given)
                              : std::format("unexpected {}, expected {}", given, quoted(keys)));
        }
        _entries.emplace_back(*name, std::move(item));
    }
}

FormTable::FormTable(std::string context) : _context{std::move(context)} {}

const std::string& FormTable::getContext() const noexcept { return _context; }

std::size_t FormTable::size() const noexcept { return _entries.size(); }

bool FormTable::has(const std::string_view key) const noexcept { return find(key) != nullptr; }

const luau::Value* FormTable::find(const std::string_view key) const noexcept {
    const auto found =
        std::ranges::find(_entries, key, [](const auto& entry) -> std::string_view { return entry.first; });
    return found == _entries.end() ? nullptr : &found->second;
}

std::optional<bool> FormTable::boolean(const std::string_view key) const {
    const luau::Value* value = find(key);
    if (value == nullptr) {
        return std::nullopt;
    }
    const auto* flag = value->tryAs<bool>();
    if (flag == nullptr) {
        fail(std::format("'{}' must be a boolean", key));
    }
    return *flag;
}

std::optional<double> FormTable::number(const std::string_view key) const {
    const luau::Value* value = find(key);
    if (value == nullptr) {
        return std::nullopt;
    }
    const auto* number = value->tryAs<double>();
    if (number == nullptr) {
        fail(std::format("'{}' must be a number", key));
    }
    return *number;
}

std::optional<std::uint32_t> FormTable::count(const std::string_view key) const {
    const luau::Value* value = find(key);
    if (value == nullptr) {
        return std::nullopt;
    }
    const auto count = value->toUInt32();
    if (!count) {
        fail(std::format("'{}' must be a whole number, 0 or more", key));
    }
    return count;
}

std::optional<std::string> FormTable::string(const std::string_view key) const {
    const luau::Value* value = find(key);
    if (value == nullptr) {
        return std::nullopt;
    }
    const auto* text = value->tryAs<std::string>();
    if (text == nullptr) {
        fail(std::format("'{}' must be a string", key));
    }
    return *text;
}

std::optional<luau::Value> FormTable::vec2(const std::string_view key) const {
    const luau::Value* value = find(key);
    if (value == nullptr) {
        return std::nullopt;
    }
    const std::string context = std::format("{}: '{}'", _context, key);
    if (!value->is<luau::Table>()) {
        throw luau::exceptions::RuntimeError{std::format("{} must be a table {{x, y}}", context)};
    }
    const FormTable vector{context, *value, kVec2Keys};
    for (const std::string_view axis : kVec2Keys) {
        if (!vector.number(axis)) {
            vector.fail(std::format("missing '{}'", axis));
        }
    }
    return *value;
}

std::optional<std::vector<luau::Value>> FormTable::list(const std::string_view key) const {
    const luau::Value* value = find(key);
    if (value == nullptr) {
        return std::nullopt;
    }
    return readList(std::format("{}: '{}'", _context, key), *value);
}

void FormTable::fail(const std::string_view what) const {
    throw luau::exceptions::RuntimeError{std::format("{}: {}", _context, what)};
}

std::vector<luau::Value> readList(const std::string_view context, const luau::Value& value) {
    std::vector<std::pair<std::uint32_t, luau::Value>> indexed;
    for (auto& [key, item] : entriesOf(context, value)) {
        const auto index = key.toUInt32();
        if (!index || *index == 0) {
            throw luau::exceptions::RuntimeError{
                std::format("{}: expected a list, without holes or named keys", context)};
        }
        indexed.emplace_back(*index, std::move(item));
    }
    // The keys are distinct, so they are 1 to n when the largest is n.
    std::ranges::sort(indexed, {}, &std::pair<std::uint32_t, luau::Value>::first);
    if (!indexed.empty() && indexed.back().first != indexed.size()) {
        throw luau::exceptions::RuntimeError{std::format("{}: expected a list, without holes or named keys", context)};
    }
    std::vector<luau::Value> items;
    items.reserve(indexed.size());
    for (auto& [index, item] : indexed) {
        items.push_back(std::move(item));
    }
    return items;
}

std::vector<std::pair<std::string, luau::Value>> readNamedEntries(const std::string_view context,
                                                                  const luau::Value& value) {
    std::vector<std::pair<std::string, luau::Value>> named;
    for (auto& [key, item] : entriesOf(context, value)) {
        auto* name = key.tryAs<std::string>();
        if (name == nullptr) {
            throw luau::exceptions::RuntimeError{std::format("{}: every key must be a name", context)};
        }
        named.emplace_back(std::move(*name), std::move(item));
    }
    std::ranges::sort(named, {}, [](const auto& entry) -> const std::string& { return entry.first; });
    return named;
}

void checkArgumentCount(const std::string_view form, const luau::CallContext& context, const std::size_t count) {
    if (context.argCount() > count) {
        throw luau::exceptions::RuntimeError{
            std::format("{}: expected at most {} argument(s), got {}", form, count, context.argCount())};
    }
}

luau::Value argumentAt(const luau::CallContext& context, const std::size_t index) {
    const std::span<const luau::Value> arguments = context.args();
    return index < arguments.size() ? arguments.subspan(index, 1).front() : luau::Value{luau::Value::Nil{}};
}
}  // namespace rtype::scripting::setup
