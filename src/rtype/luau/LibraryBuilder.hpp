/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LibraryBuilder
*/

#pragma once

#include <string>

#include "Export.hpp"
#include "Function.hpp"
#include "Library.hpp"
#include "Table.hpp"

namespace rtype::luau {
class Runtime;

/// @brief Builds a Library step by step: sets its name and registers its members.
///
/// The builder is single-use: build() moves its table and members into the library.
class RTYPE_LUAU_API LibraryBuilder {
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

    /// @brief Registers a library as a sub-library of this one, under its own name in the library table.
    /// @param library Library to register. The builder takes ownership, so it stays alive as long as the member exists,
    /// including after build().
    /// @see `linkLibrary` to link a library without taking ownership.
    void adoptLibrary(Library library);

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

    /// Owns the members registered in the table so they outlive the builder's callers.
    LibraryMembers _ownedMembers;
};
}  // namespace rtype::luau
