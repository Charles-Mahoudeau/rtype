/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Function
*/

#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "CallContext.hpp"
#include "Ref.hpp"

struct lua_State;

namespace rtype::luau {
/// @brief Handle on a Luau function, kept alive through a registry reference.
///
/// A Function either wraps a native closure made by `create`, which forwards its calls from Lua to a C++
/// handler, or a function defined in Luau. The native closure stores the address of its Function in its first
/// upvalue: moving the Function rebinds that upvalue to the new address, and destroying it unbinds the closure,
/// so a call from Lua then raises a Lua error instead of reaching a dangling Function. The referenced
/// `lua_State` must outlive the Function.
class RTYPE_LUAU_API Function {
  public:
    /// @brief Where the body of the wrapped function lives.
    enum class BindingType : std::uint8_t {
        kNative,  ///< Native closure forwarding its calls to the C++ handler.
        kScript,  ///< Function defined in Luau.
    };

    /// @brief C++ callable run each time Lua calls the native closure.
    using Handler = std::move_only_function<void(const CallContext&)>;

    /// @brief Creates a native closure bound to a new Function and registers it as a global of the Lua state.
    ///
    /// The closure has no handler yet: calling it from Lua raises a Lua error until `setHandler` gives it one.
    /// @param state The Lua state to create the closure in.
    /// @param name Name of the global the closure is stored in, also used as its debug name and in error messages.
    /// @return The Function bound to the new closure.
    [[nodiscard]] static Function create(lua_State* state, std::string name);

    /// @brief Wraps a function defined in Luau, so C++ can call it.
    /// @param ref Reference to the Luau function; the Function takes it over and lives in its Lua state.
    /// @param name Name of the function, used in error messages.
    /// @return The Function wrapping the referenced function.
    [[nodiscard]] static Function fromRef(Ref ref, std::string name);

    /// @brief Unbinds the native closure from this Function, so calling it from Lua raises an error.
    ~Function();

    /// @brief Copy is deleted: the native closure is bound to a single Function.
    Function(const Function&) = delete;

    /// @brief Copy is deleted: the native closure is bound to a single Function.
    Function& operator=(const Function&) = delete;

    /// @brief Takes over another Function and rebinds its native closure to this one.
    /// @param other The Function to move from; it references nothing afterwards.
    Function(Function&& other) noexcept;

    /// @brief Unbinds the current native closure, then takes over another Function and rebinds its closure.
    /// @param other The Function to move from; it references nothing afterwards.
    /// @return A reference to this Function.
    Function& operator=(Function&& other) noexcept;

    /// @brief Gives access to the registry reference to the wrapped function.
    /// @return The reference, empty if the Function was moved from.
    [[nodiscard]] const Ref& ref() const noexcept;

    /// @brief Sets the C++ handler run when Lua calls the native closure, replacing the previous one.
    /// @param handler The handler to run; an empty one makes calls from Lua raise a Lua error.
    void setHandler(Handler handler);

    /// @brief Calls the wrapped function in protected mode, without arguments, discarding its results.
    ///
    /// Prints a warning on `std::cerr` if the Function wraps a native closure.
    /// @param context Context of the call.
    /// @throws exceptions::InvalidRef If the Function was moved from.
    /// @throws exceptions::RuntimeError If the function raised an error.
    void call(CallContext context) const;

  private:
    /// @brief Creates a Function wrapping an existing function.
    /// @param state The Lua state the function lives in.
    /// @param type Where the body of the function lives.
    /// @param ref Reference to the function.
    /// @param name Name used in error messages.
    Function(lua_State* state, BindingType type, Ref ref, std::string name = "unknown");

    /// @brief Creates a native Function that references no closure yet; `create` makes and binds it.
    /// @param state The Lua state the closure will live in.
    /// @param type Where the body of the function lives.
    /// @param name Name used in error messages.
    Function(lua_State* state, BindingType type, std::string name);

    /// @brief Gives the name as a C string.
    /// @return The name, valid until the name changes or the Function is destroyed.
    [[nodiscard]] const char* cStrName() const noexcept;

    /// @brief Sets the name used in error messages.
    /// @param name The new name.
    void setName(std::string name);

    /// @brief Sets the registry reference to the native closure wrapped by this Function.
    /// @param ref The reference to the closure, made by `create`.
    /// @throws exceptions::InvalidOperation If the Function wraps a function defined in Luau.
    void setRef(Ref ref);

    /// @brief Points the native closure's upvalue to @p to, if it still points to @p from.
    ///
    /// Does nothing if the Function is empty or wraps a closure that `create` did not make.
    /// @param from The Function the upvalue is expected to point to.
    /// @param to The Function to point the upvalue to, or nullptr to unbind it.
    void rebindUpvalue(const Function* from, Function* to) const noexcept;

    /// @brief C function of every closure made by `create`: forwards the call to the Function stored in its
    ///        first upvalue.
    ///
    /// Raises a Lua error if that Function was destroyed or has no handler.
    /// @param state The Lua state calling the closure.
    /// @return The number of values returned to Lua.
    /// @throws exceptions::InvalidOperation If the upvalue points to a Function wrapping a function defined in Luau.
    static int handle(lua_State* state);

    lua_State* _state;  ///< Lua state the function lives in.
    BindingType _type;  ///< Where the body of the wrapped function lives.
    Ref _ref;           ///< Registry reference to the function.
    std::string _name;  ///< Name used in error messages.
    Handler _handler;   ///< C++ handler called by the native closure.
};
}  // namespace rtype::luau
