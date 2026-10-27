/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <variant>

#include "Export.hpp"
#include "Function.hpp"

namespace rtype::luau {
class Table;
class LibraryBuilder;

/// @brief A named Luau library: a table of members (functions...) that scripts can use.
///
/// A library owns its table and the members registered in it, so they live as long as the library does.
/// It is produced by a LibraryBuilder.
class RTYPE_LUAU_API Library {
  public:
    /// @brief Something a library exports and owns.
    using Member = std::variant<Function, Library>;

    /// @brief Builds a library from its parts.
    /// @param name Name of the library.
    /// @param table Table holding the library's members, indexed by name.
    /// @param ownedMembers Members referenced by the table, kept here so they stay alive.
    Library(std::string name, Table table, std::deque<Member> ownedMembers);
    ~Library();
    Library(const Library&) = delete;
    Library& operator=(const Library&) = delete;
    Library(Library&&) noexcept;
    Library& operator=(Library&&) noexcept;

    /// @brief Gets the name of the library.
    /// @return A view on the name, valid as long as the library is alive and not modified.
    [[nodiscard]] std::string_view name() const;

    /// @brief Gets the table holding the members of the library.
    /// @return The library table.
    [[nodiscard]] const Table& table() const;

  private:
    std::string _name;  ///< Name of the library.
    Table _table;       ///< Table of the library's members, indexed by name.
    /// Owns the members referenced by the table. A deque keeps references handed out by the builder valid.
    std::deque<Member> _ownedMembers;
};
}  // namespace rtype::luau
