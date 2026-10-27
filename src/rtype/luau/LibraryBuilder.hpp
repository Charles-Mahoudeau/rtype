/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LibraryBuilder
*/

#pragma once

#include <deque>
#include <string>

#include "Function.hpp"
#include "Library.hpp"
#include "Table.hpp"

namespace rtype::luau {
class Runtime;

/// @brief Builds a Library step by step: sets its name and registers its members.
///
/// The builder is single-use: build() moves its table and members into the library.
class LibraryBuilder {
  public:
    /// @brief Creates a builder with an empty library table.
    /// @param runtime Runtime the library belongs to, it must outlive the builder.
    /// @throws exceptions::RuntimeError If the library table cannot be created.
    explicit LibraryBuilder(Runtime& runtime);
    ~LibraryBuilder() = default;
    LibraryBuilder(const LibraryBuilder&) = delete;
    LibraryBuilder& operator=(const LibraryBuilder&) = delete;
    LibraryBuilder(LibraryBuilder&&) noexcept = delete;
    LibraryBuilder& operator=(LibraryBuilder&&) noexcept = delete;

    /// @brief Produces the library, moving the table and members out of the builder.
    /// @return The built library. The builder must not be used afterwards.
    [[nodiscard]] Library build();

    /// @brief Sets the name of the library, also stored as `__name` in its table.
    /// @param name Name of the library.
    void setName(const std::string& name);

    /// @brief Creates a function and registers it in the library table under its name.
    /// @param name Name of the function, used as its key in the library table.
    /// @return The new function. The reference stays valid as long as the member exists, including after build().
    [[nodiscard]] Function& function(std::string name);

  private:
    /// @brief Gets the library table being built.
    /// @return The library table.
    [[nodiscard]] Table& table();

    Runtime& _runtime;  ///< Runtime the library belongs to.
    Table _table;       ///< Table of the library's members, moved into the library by build().
    std::string _name;  ///< Name of the library.

    /// Owns the members registered in the table so they outlive the builder's callers. A deque keeps the
    /// references returned by function() valid when more members are added.
    std::deque<Library::Member> _ownedMembers;
};
}  // namespace rtype::luau
