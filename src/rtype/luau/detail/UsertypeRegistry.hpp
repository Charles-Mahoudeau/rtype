/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UsertypeRegistry
*/

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "StateLife.hpp"
#include "UsertypeData.hpp"
#include "rtype/luau/Export.hpp"
#include "rtype/luau/Result.hpp"

struct lua_State;

namespace rtype::luau::detail {
/// @brief The usertypes registered in one Lua state, by C++ type.
///
/// Owned by the `Runtime`, which is its only creator. It hands out the userdata tags (each type has its own, as the
/// destructor of a tag is per type) and reaches the state back through `of()`, so a `Userdata` can tell whether it
/// holds a given C++ type.
class RTYPE_LUAU_API UsertypeRegistry {
  public:
    /// @brief Gets the registry of a Lua state.
    /// @param state Any thread of the state.
    /// @note The callbacks' `userdata` slot of a state belongs to its `Runtime`: host code must leave it alone, as a
    ///       foreign pointer there cannot be told from a registry.
    /// @return The registry, or `nullptr` if the state was not created by a `Runtime`.
    [[nodiscard]] static UsertypeRegistry* of(lua_State* state) noexcept;

    /// @brief Creates the registry of a Lua state, and makes `of()` find it.
    /// @param state The main thread of the Lua state; must not be null. The state must stay open until the
    ///        registry is destroyed, because the destructor reads it.
    /// @throws exceptions::InvalidState If @p state is null.
    explicit UsertypeRegistry(lua_State* state);
    /// @brief Marks the state as closed for the handles that outlive it (see StateLife).
    ~UsertypeRegistry();
    UsertypeRegistry(const UsertypeRegistry&) = delete;
    UsertypeRegistry& operator=(const UsertypeRegistry&) = delete;
    UsertypeRegistry(UsertypeRegistry&&) noexcept = delete;
    UsertypeRegistry& operator=(UsertypeRegistry&&) noexcept = delete;

    /// @return Whether the state lives, shared with the handles on its values; they keep it after the registry.
    [[nodiscard]] std::shared_ptr<const StateLife> life() const noexcept;

    /// @brief Finds the usertype of a C++ type.
    /// @param type The C++ type.
    /// @return The usertype, or `nullptr` if the type is not registered.
    [[nodiscard]] UsertypeData* find(std::type_index type) const noexcept;

    /// @brief Registers a C++ type.
    /// @param type The C++ type.
    /// @param name Name of the type for scripts.
    /// @param size Size in bytes of an object of the type.
    /// @param destructor Destroys an object of the type.
    /// @return The usertype, valid as long as the registry, or an `ErrorKind::kLimitExceeded` error if the Lua state
    /// has no
    ///         userdata tag left.
    /// @throws exceptions::InvalidOperation If the type is already registered.
    [[nodiscard]] Result<UsertypeData*> add(std::type_index type, std::string name, std::size_t size,
                                            UsertypeData::Destructor destructor);

  private:
    lua_State* _state;                                                ///< Main thread of the Lua state.
    std::shared_ptr<StateLife> _life{std::make_shared<StateLife>()};  ///< Whether the state lives.
    int _nextTag{1};                                                  ///< Next free userdata tag (0 is untagged).
    std::unordered_map<std::type_index, std::unique_ptr<UsertypeData>> _types;  ///< Usertypes by C++ type.
};
}  // namespace rtype::luau::detail
