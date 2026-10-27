/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#pragma once

#include <deque>
#include <memory>
#include <string>
#include <string_view>

#include "Export.hpp"
#include "Function.hpp"

namespace rtype::luau {
class Table;
class LibraryBuilder;
class Library;

/// @brief What a library owns: the members referenced by its table, grouped by kind.
///
/// Sub-libraries are held through pointers because Library is incomplete here, and a deque of an incomplete type is
/// not allowed. Deques keep the references handed out by the builder valid when more members are added.
struct LibraryMembers {
    std::deque<Function> functions;                  ///< Functions of the library.
    std::deque<std::unique_ptr<Library>> libraries;  ///< Sub-libraries of the library.
};

/// @brief A named Luau library: a table of members (functions...) that scripts can use.
///
/// A library owns its table and the members registered in it, so they live as long as the library does.
/// It is produced by a LibraryBuilder.
class RTYPE_LUAU_API Library {
  public:
    /// @brief Builds a library from its parts.
    /// @param name Name of the library.
    /// @param table Table holding the library's members, indexed by name.
    /// @param ownedMembers Members referenced by the table, kept here so they stay alive.
    Library(std::string name, Table table, LibraryMembers ownedMembers);
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
    std::string _name;             ///< Name of the library.
    Table _table;                  ///< Table of the library's members, indexed by name.
    LibraryMembers _ownedMembers;  ///< Owns the members referenced by the table.
};
}  // namespace rtype::luau
