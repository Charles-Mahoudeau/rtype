/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FormTable
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/Value.hpp"

namespace rtype::scripting::setup {
/// @brief A table a setup form reads, whose keys are all known in advance: the options of `engine.assets.texture`, a
/// frame of `engine.assets.atlas`...
///
/// @details Every error is a luau::exceptions::RuntimeError whose message starts with the table's context, so a
/// script sees which form, and which part of it, is wrong: `engine.assets.atlas: frame 2: 'size' must be a table
/// {x, y}`. A `nil` value reads as absent.
class FormTable {
  public:
    /// @brief Reads an optional table: `nil` reads as an empty one.
    /// @param context What the table is, for the messages.
    /// @param value The table, or `nil`.
    /// @param keys The keys it may have.
    /// @return The table read.
    /// @throws luau::exceptions::RuntimeError As the constructor.
    [[nodiscard]] static FormTable optional(std::string context, const luau::Value& value,
                                            std::span<const std::string_view> keys);
    /// @brief Reads a table whose keys must all be in @p keys.
    /// @param context What the table is, for the messages ("engine.assets.atlas", "engine.assets.atlas: grid").
    /// @param value The table.
    /// @param keys The keys it may have.
    /// @throws luau::exceptions::RuntimeError If @p value is not a table, or has a key that is not one of @p keys.
    FormTable(std::string context, const luau::Value& value, std::span<const std::string_view> keys);
    ~FormTable() = default;
    FormTable(const FormTable&) = default;
    FormTable& operator=(const FormTable&) = default;
    FormTable(FormTable&&) noexcept = default;
    FormTable& operator=(FormTable&&) noexcept = default;

    /// @return What the table is, the start of every message.
    [[nodiscard]] const std::string& getContext() const noexcept;

    /// @return The number of keys the table has.
    [[nodiscard]] std::size_t size() const noexcept;
    /// @return Whether the table has @p key.
    [[nodiscard]] bool has(std::string_view key) const noexcept;
    /// @return The value of @p key, or nullptr when the table does not have it.
    [[nodiscard]] const luau::Value* find(std::string_view key) const noexcept;

    /// @return The boolean of @p key, if the table has it.
    /// @throws luau::exceptions::RuntimeError If it is not a boolean.
    [[nodiscard]] std::optional<bool> boolean(std::string_view key) const;
    /// @return The number of @p key, if the table has it.
    /// @throws luau::exceptions::RuntimeError If it is not a number.
    [[nodiscard]] std::optional<double> number(std::string_view key) const;
    /// @return The whole number of @p key, if the table has it.
    /// @throws luau::exceptions::RuntimeError If it is not a whole number between 0 and 2^32 - 1.
    [[nodiscard]] std::optional<std::uint32_t> count(std::string_view key) const;
    /// @return The string of @p key, if the table has it.
    /// @throws luau::exceptions::RuntimeError If it is not a string.
    [[nodiscard]] std::optional<std::string> string(std::string_view key) const;
    /// @return The `{x, y}` table of @p key, as it is, if the table has it.
    /// @throws luau::exceptions::RuntimeError If it is not a table of the numbers `x` and `y` and nothing else.
    [[nodiscard]] std::optional<luau::Value> vec2(std::string_view key) const;
    /// @return The items of the list of @p key, in order, if the table has it.
    /// @throws luau::exceptions::RuntimeError If it is not a list: a table with holes or keys that are not 1 to n.
    [[nodiscard]] std::optional<std::vector<luau::Value>> list(std::string_view key) const;

    /// @brief Fails with a message about the table.
    /// @param what What is wrong, after the context.
    /// @throws luau::exceptions::RuntimeError `<context>: <what>`, always.
    [[noreturn]] void fail(std::string_view what) const;

  private:
    /// @brief An empty table.
    explicit FormTable(std::string context);

    std::string _context;                                       ///< What the table is, the start of every message.
    std::vector<std::pair<std::string, luau::Value>> _entries;  ///< Its keys and values, in no particular order.
};

/// @brief Reads the items of a list, in order.
/// @param context What the list is, for the messages.
/// @param value The list.
/// @return Its items.
/// @throws luau::exceptions::RuntimeError `<context>: ...` if @p value is not a table, or has holes or keys that are
/// not 1 to n.
[[nodiscard]] std::vector<luau::Value> readList(std::string_view context, const luau::Value& value);

/// @brief Reads the entries of a table whose keys are all strings, sorted by key: the animations of an atlas, by name.
/// @param context What the table is, for the messages.
/// @param value The table.
/// @return Its entries, sorted by key.
/// @throws luau::exceptions::RuntimeError `<context>: ...` if @p value is not a table or has a key that is not a
/// string.
[[nodiscard]] std::vector<std::pair<std::string, luau::Value>> readNamedEntries(std::string_view context,
                                                                                const luau::Value& value);

/// @brief Checks that a form was given at most @p count arguments.
/// @param form The form, the start of the message.
/// @param context The arguments it was called with.
/// @param count How many it takes at most.
/// @throws luau::exceptions::RuntimeError `<form>: ...` if it was given more.
void checkArgumentCount(std::string_view form, const luau::CallContext& context, std::size_t count);

/// @return The argument at @p index (from 0) a form was called with, or `nil` past the last one.
[[nodiscard]] luau::Value argumentAt(const luau::CallContext& context, std::size_t index);
}  // namespace rtype::scripting::setup
