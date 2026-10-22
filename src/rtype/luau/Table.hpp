/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Table
*/

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Export.hpp"
#include "MapHelper.hpp"
#include "Ref.hpp"
#include "Result.hpp"

struct lua_State;

namespace rtype::luau {
class Value;
/// @brief Handle on a table that lives in a Luau state.
///
/// Mirrors the reference semantics of Luau: copying a `Table` copies the handle, not the contents, so every copy
/// (and the scripts) see the same entries. The table stays alive as long as a handle or a script refers to it, and
/// the handle must not outlive the `Runtime` of its state.
///
/// Keys and values are `Value`s. Reads and writes are raw: metatables (`__index`, `__newindex`) are bypassed, so
/// they never run script code and never raise a Luau error. A table cannot be used from several threads at once,
/// like the state it belongs to.
class RTYPE_LUAU_API Table {
  public:
    /// @brief A key/value pair read from a table.
    using Entry = std::pair<Value, Value>;
    /// @brief String-keyed snapshot of a table, with transparent string hashing and comparison.
    using Map = std::unordered_map<std::string, Value, StringHash, std::equal_to<>>;

    /// @brief Creates a new, empty table in a Lua state.
    /// @param state The Lua state to create the table in; must not be null.
    /// @param arrayCapacity Number of array slots to preallocate.
    /// @param hashCapacity Number of hash entries to preallocate.
    /// @return A handle on the new table, or a Failure of kind `kStackOverflow` if the stack of @p state cannot grow
    ///         by one slot.
    /// @throws exceptions::InvalidState If @p state is null.
    [[nodiscard]] static Result<Table> create(lua_State* state, int arrayCapacity = 0, int hashCapacity = 0);
    /// @brief Makes a handle on the table at a stack index of a Lua state.
    ///
    /// Leaves the stack untouched. The reverse of `push()`.
    /// @param state The Lua state to read from; must not be null.
    /// @param stackIndex Stack index of the table (defaults to the top). It must be a valid index of @p state's
    ///        stack.
    /// @return A handle on the table, or a Failure of kind `kTypeMismatch` if the value is not a table.
    [[nodiscard]] static Result<Table> fromStack(lua_State* state, int stackIndex = Ref::kLuaStackTop);

    /// @brief Destroys the handle. The table itself lives on while another handle or a script refers to it.
    ~Table() noexcept;
    /// @brief Copy constructor: the new handle refers to the same table.
    Table(const Table&) noexcept;
    /// @brief Copy assignment: the handle refers to the same table as the other one.
    Table& operator=(const Table&) noexcept;
    /// @brief Move constructor: takes over the handle. The moved-from handle can only be destroyed or assigned to.
    Table(Table&&) noexcept;
    /// @brief Move assignment: takes over the handle. The moved-from handle can only be destroyed or assigned to.
    Table& operator=(Table&&) noexcept;

    /// @brief Reads the value stored under a key.
    /// @param key Key to look up.
    /// @return The value (`nil` if the key is absent), or a Failure if the stack cannot grow, or of kind
    ///         `kTypeMismatch` if the value has a type that cannot be read yet (see `Value::fromStack`).
    [[nodiscard]] Result<Value> get(const Value& key) const;
    /// @brief Reads the value stored under a string key.
    /// @param key Key to look up.
    /// @return Same as `get(const Value&)`.
    [[nodiscard]] Result<Value> get(std::string_view key) const;
    /// @brief Reads the value stored at an array index.
    /// @param index 1-based index, matching the Luau convention.
    /// @return Same as `get(const Value&)`.
    [[nodiscard]] Result<Value> getAt(std::size_t index) const;

    /// @brief Stores a value under a key, overwriting the existing one. Storing `nil` removes the entry.
    /// @param key Key to store the value under; cannot be `nil` or NaN.
    /// @param value Value to store.
    /// @return Nothing on success, or a Failure of kind `kTypeMismatch` for an invalid key, of kind `kRuntime` if
    ///         the table is read-only or the value cannot be pushed, or if the stack cannot grow.
    [[nodiscard]] Result<void> set(const Value& key, const Value& value);
    /// @brief Stores a value under a string key.
    /// @param key Key to store the value under.
    /// @param value Value to store.
    /// @return Same as `set(const Value&, const Value&)`.
    [[nodiscard]] Result<void> set(std::string_view key, const Value& value);
    /// @brief Stores a value at an array index.
    /// @param index 1-based index, matching the Luau convention.
    /// @param value Value to store.
    /// @return Same as `set(const Value&, const Value&)`.
    [[nodiscard]] Result<void> setAt(std::size_t index, const Value& value);

    /// @brief Checks whether a key has a value (not `nil`) in the table.
    /// @param key Key to look up.
    /// @return `true` if the key is present, or a Failure if the stack cannot grow.
    [[nodiscard]] Result<bool> has(const Value& key) const;
    /// @brief Checks whether a string key has a value (not `nil`) in the table.
    /// @param key Key to look up.
    /// @return Same as `has(const Value&)`.
    [[nodiscard]] Result<bool> has(std::string_view key) const;

    /// @brief Returns the length of the array part, like the `#` operator without its metamethod.
    ///
    /// With holes in the array, any border is a valid answer, as in Luau.
    /// @return The length, or a Failure of kind `kStackOverflow` if the stack cannot grow by one slot.
    [[nodiscard]] Result<std::size_t> size() const;
    /// @brief Returns the number of entries of the table, array and hash parts together.
    ///
    /// Walks the whole table: linear in its size.
    /// @return The number of entries, or a Failure of kind `kStackOverflow` if the stack cannot grow enough.
    [[nodiscard]] Result<std::size_t> count() const;

    /// @brief Copies every entry of the table, in the unspecified order of `pairs()`.
    ///
    /// The snapshot is taken before returning, so the table can be modified while going through it. Nested
    /// tables are handles on the live tables, not copies.
    /// @return The entries, or a Failure if the stack cannot grow, or of kind `kTypeMismatch` if a key or a value
    ///         has a type that cannot be read yet (see `Value::fromStack`).
    [[nodiscard]] Result<std::vector<Entry>> entries() const;
    /// @brief Copies the entries of the table into a string-keyed map (a shallow snapshot, like `entries()`).
    /// @return The map, or a Failure of kind `kTypeMismatch` if a key is not a string, or the failure of
    ///         `entries()`.
    [[nodiscard]] Result<Map> toMap() const;

    /// @brief Pushes the table onto the stack of a Lua state.
    ///
    /// Pushes exactly one value on success and nothing on failure. Does not grow the stack: the caller makes sure
    /// there is room for one more slot (`lua_checkstack`).
    /// @param state The Lua state to push onto; must not be null.
    /// @return Nothing on success, or a Failure of kind `kRuntime` if @p state belongs to another Lua state than the
    ///         table.
    /// @throws exceptions::InvalidState If @p state is null.
    /// @throws exceptions::InvalidRef If the handle was moved from.
    [[nodiscard]] Result<void> push(lua_State* state) const;

    /// @brief Checks whether two handles refer to the same table (raw identity, not equal contents).
    /// @param other The handle to compare with.
    /// @return `true` if they do (`false` for a table of another Lua state), or a Failure of kind `kStackOverflow`
    ///         if the stack cannot grow enough.
    /// @throws exceptions::InvalidRef If a handle was moved from.
    [[nodiscard]] Result<bool> refersToSame(const Table& other) const;

  private:
    /// @brief Wraps a reference to a table.
    /// @param ref Registry reference to the table.
    explicit Table(Ref ref);

    /// @brief Returns the main thread of the table's state.
    /// @throws exceptions::InvalidRef If the handle was moved from.
    [[nodiscard]] lua_State* state() const;

    std::shared_ptr<Ref> _ref;  ///< Registry reference to the table, shared by every copy of the handle.
};
}  // namespace rtype::luau
