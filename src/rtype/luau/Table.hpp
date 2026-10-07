/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Table
*/

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "MapHelper.hpp"
#include "Value.hpp"

namespace rtype::luau {
class Value;
/// @brief String-keyed associative container holding Luau values.
///
/// Models a Luau table whose keys are strings. Lookups accept `std::string_view`
/// without allocating a temporary `std::string` (heterogeneous lookup).
class Table {
  public:
    /// @brief Underlying map type, with transparent string hashing and comparison.
    using Map = std::unordered_map<std::string, Value, StringHash, std::equal_to<>>;
    /// @brief Mutable iterator over the key/value pairs.
    using iterator = Map::iterator;
    /// @brief Read-only iterator over the key/value pairs.
    using const_iterator = Map::const_iterator;

    /// @brief Constructs an empty table.
    Table();
    /// @brief Destroys the table and every value it holds.
    ~Table() noexcept;
    /// @brief Copy constructor: duplicates every entry of @p other.
    Table(const Table&) noexcept;
    /// @brief Copy assignment: replaces the content with a copy of the other table's entries.
    Table& operator=(const Table&) noexcept;
    /// @brief Move constructor: takes over the entries of the other table.
    Table(Table&&) noexcept;
    /// @brief Move assignment: takes over the entries of the other table.
    Table& operator=(Table&&) noexcept;

    /// @brief Looks up a value by key (read-only).
    /// @param key Key to search for.
    /// @return Pointer to the value, or `nullptr` if the key is absent. The pointer is invalidated
    ///         by any operation that modifies the table structure (e.g. `set`).
    [[nodiscard]] const Value* tryGet(std::string_view key) const noexcept;
    /// @brief Looks up a value by key (mutable).
    /// @param key Key to search for.
    /// @return Pointer to the value, or `nullptr` if the key is absent. The pointer is invalidated
    ///         by any operation that modifies the table structure (e.g. `set`).
    [[nodiscard]] Value* tryGet(std::string_view key) noexcept;

    /// @brief Inserts a value, or overwrites the existing one if the key is already present.
    /// @param key Key to associate with the value.
    /// @param value Value to store.
    /// @return Reference to the stored value.
    Value& set(std::string key, Value value);

    /// @brief Returns the number of entries in the table.
    [[nodiscard]] std::size_t size() const noexcept;

    /// @brief Returns an iterator to the first entry. Iteration order is unspecified.
    [[nodiscard]] iterator begin() noexcept;
    /// @brief Returns an iterator past the last entry.
    [[nodiscard]] iterator end() noexcept;
    /// @brief Returns a read-only iterator to the first entry. Iteration order is unspecified.
    [[nodiscard]] const_iterator begin() const noexcept;
    /// @brief Returns a read-only iterator past the last entry.
    [[nodiscard]] const_iterator end() const noexcept;
    /// @brief Returns a read-only iterator to the first entry. Iteration order is unspecified.
    [[nodiscard]] const_iterator cbegin() const noexcept;
    /// @brief Returns a read-only iterator past the last entry.
    [[nodiscard]] const_iterator cend() const noexcept;

  private:
    Map _map;  ///< Entries of the table, indexed by key.
};
}  // namespace rtype::luau
