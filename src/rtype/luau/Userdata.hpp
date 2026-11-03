/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Userdata
*/

#pragma once

#include <memory>
#include <typeindex>

#include "Export.hpp"
#include "Ref.hpp"
#include "Result.hpp"

struct lua_State;

namespace rtype::luau {
/// @brief A handle on a userdata of a Lua state: a C++ object of a type registered with `Runtime::usertype()`.
///
/// Like `Table` and `FunctionRef`, it is a registry reference, not a copy: every copy of the handle designates the
/// same userdata, which stays alive as long as one handle (or the Lua state itself) refers to it. The C++ object is
/// reached with `tryAs()`.
class RTYPE_LUAU_API Userdata {
  public:
    /// @brief Creates a handle on the userdata at a stack index of a Lua state.
    ///
    /// Leaves the stack untouched.
    /// @param state The Lua state to read from; must not be null.
    /// @param stackIndex Stack index of the userdata (defaults to the top). It must be a valid index of @p state's
    ///        stack.
    /// @return A handle on the userdata, or a Failure of kind `kTypeMismatch` if the value is not a userdata.
    /// @throws exceptions::InvalidState If @p state is null.
    [[nodiscard]] static Result<Userdata> fromStack(lua_State* state, int stackIndex = Ref::kLuaStackTop);

    /// @brief Destroys the handle, releasing its reference to the userdata.
    ~Userdata() noexcept = default;
    /// @brief Copy constructor: the copy designates the same userdata.
    Userdata(const Userdata&) noexcept = default;
    /// @brief Copy assignment: the handle then designates the same userdata as @p other.
    Userdata& operator=(const Userdata&) noexcept = default;
    /// @brief Move constructor.
    Userdata(Userdata&&) noexcept = default;
    /// @brief Move assignment.
    Userdata& operator=(Userdata&&) noexcept = default;

    /// @brief Checks whether the handle designates a userdata (it does not once moved from).
    /// @return `true` if the handle holds a valid reference.
    [[nodiscard]] bool isValid() const noexcept;

    /// @brief Gets the tag of the userdata, which identifies its usertype in the Lua state.
    /// @return The tag.
    /// @throws exceptions::InvalidRef If the handle is empty.
    [[nodiscard]] int tag() const;

    /// @brief Gets the C++ object held by the userdata.
    /// @tparam T The type of the object; it must have been registered with `Runtime::usertype()`.
    /// @return A pointer to the object, valid as long as a handle on the userdata is alive, or `nullptr` if the
    ///         userdata holds an object of another type (or of a type of another runtime).
    /// @throws exceptions::InvalidRef If the handle is empty.
    template <typename T>
    [[nodiscard]] T* tryAs() const;

    /// @brief Pushes the userdata onto the stack of a Lua state.
    ///
    /// Pushes exactly one value on success and nothing on failure.
    /// @param state The Lua state to push onto; must not be null.
    /// @return Nothing on success, or a Failure of kind `kRuntime` if the userdata belongs to another Lua state.
    /// @throws exceptions::InvalidState If @p state is null.
    /// @throws exceptions::InvalidRef If the handle is empty.
    [[nodiscard]] Result<void> push(lua_State* state) const;

  private:
    /// @brief Creates a handle from a reference to a userdata.
    /// @param ref Registry reference to the userdata.
    explicit Userdata(Ref ref);

    /// @brief Gets the object held by the userdata, if it is of the given type.
    /// @param type The C++ type the object must have.
    /// @return A pointer to the object, or `nullptr` if it has another type.
    [[nodiscard]] void* pointerOf(std::type_index type) const;

    std::shared_ptr<Ref> _ref;  ///< Registry reference to the userdata, shared by every copy of the handle.
};
}  // namespace rtype::luau

#define RTYPE_USERDATA_INCLUDING_TPP
#include "Userdata.tpp"  // IWYU pragma: export
#undef RTYPE_USERDATA_INCLUDING_TPP
