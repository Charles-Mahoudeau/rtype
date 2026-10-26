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
#include "Export.hpp"
#include "FunctionRef.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "ValuePack.hpp"

struct lua_State;

namespace rtype::luau {
/// @brief Handle on a Luau function, kept alive through a registry reference.
///
/// A Function either wraps a native closure made by `create`, which forwards its calls from Lua to a C++
/// handler, or a function defined in Luau, which C++ calls with `call`. Both directions exchange their arguments and
/// return values as `ValuePack`s. The native closure stores the address of its Function in its first
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

    /// @brief C++ callable run each time Lua calls the native closure, returning the values of the call.
    ///
    /// The context holds the arguments the closure was called with, and the values of the returned pack are
    /// returned to Lua, in order. If an argument cannot be read (see `CallContext::fromStack`), the handler does not
    /// run and the call raises a Lua error instead. A handler raises a Lua error by throwing an `exceptions::Base`,
    /// whose message is then prefixed with the name of the function; any other `std::exception` also becomes a Lua
    /// error, with its message unchanged, and so does anything else that is thrown, with a generic message.
    using Handler = std::function<ValuePack(const CallContext&)>;

    /// @brief Creates a native closure bound to a new Function.
    ///
    /// The closure is not stored anywhere: it is only reachable through `ref()`, so store it (in a table, or in the
    /// globals) for Lua to call it. It has no handler yet: calling it from Lua raises a Lua error until `setHandler`
    /// gives it one.
    /// @param state The Lua state to create the closure in.
    /// @param name Debug name of the closure, also used in error messages.
    /// @return The Function bound to the new closure.
    [[nodiscard]] static Function create(lua_State* state, std::string name);

    /// @brief Wraps a function defined in Luau, so C++ can call it.
    /// @param ref Reference to the Luau function; the Function takes it over and lives in its Lua state.
    /// @param name Name of the function, used in error messages.
    /// @return The Function wrapping the referenced function.
    [[nodiscard]] static Function fromRef(Ref ref, std::string name);

    /// @brief Wraps a function defined in Luau, from a handle on it, so C++ can call it.
    /// @param ref Handle on the Luau function; it is left untouched and the Function gets a reference of its own.
    /// @param name Name of the function, used in error messages.
    /// @return The Function wrapping the referenced function.
    /// @throws exceptions::InvalidRef If @p ref was moved from.
    [[nodiscard]] static Function fromRef(const FunctionRef& ref, std::string name);

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

    /// @brief Gives access to the handle on the wrapped function.
    /// @return The handle, empty if the Function was moved from.
    [[nodiscard]] const FunctionRef& ref() const noexcept;

    /// @brief Sets the C++ handler run when Lua calls the native closure, replacing the previous one.
    /// @param handler The handler to run; an empty one makes calls from Lua raise a Lua error.
    void setHandler(Handler handler);

    /// @brief Sets a typed C++ handler run when Lua calls the native closure, replacing the previous one.
    ///
    /// Unlike `setHandler`, the handler takes its arguments directly instead of a `CallContext`: `bind` checks that
    /// Lua passed as many arguments as the handler has parameters and that each one holds the type of its parameter,
    /// and raises a Lua error (an `exceptions::RuntimeError` naming the faulty argument, the type it expected and the
    /// one it got) otherwise, without running the handler. The returned pack is returned to Lua, as with
    /// `setHandler`.
    ///
    /// Each parameter must be taken by value or by `const&` (not `&` or `&&`), and its type, without cv-qualifiers and
    /// reference, must be one of the alternatives of `Value::Variant` (for example `double` for a number, not `int` or
    /// `float`). Trailing parameters may be `std::optional<T>` of such a type: Lua may then omit the argument or pass
    /// `nil`, and the handler receives an empty optional. No required parameter may follow an optional one. The
    /// callable must be copy-constructible and deducible by `std::function`, so move-only captures, generic lambdas
    /// and overloaded functors are not supported.
    /// @tparam F Type of the callable, returning a `ValuePack`.
    /// @param f The callable to run; it is copied or moved into the Function.
    template <typename F>
    void bind(F&& f);

    /// @brief Calls the wrapped function in protected mode with the given arguments, and reads the values it returns.
    ///
    /// Prints a warning on `std::cerr` if the Function wraps a native closure. If the call fails, the Lua stack is
    /// left as it was before the call.
    /// @param args The arguments passed to the function, in order.
    /// @return The values returned by the function, in the order of its `return` statement, or an `Error` of kind
    ///         `ErrorKind::kRuntime` if an argument cannot be pushed (the Lua stack cannot hold every argument, or
    ///         an argument holds a type that cannot be pushed yet, see `Value::push`) or if the function raised an
    ///         error, or of kind `ErrorKind::kTypeMismatch` if a returned value cannot be read yet (see
    ///         `ValuePack::pop`).
    /// @throws exceptions::InvalidRef If the Function was moved from.
    [[nodiscard]] Result<ValuePack> call(const ValuePack& args = {}) const;

  private:
    /// @brief Creates a Function wrapping an existing function.
    /// @param state The Lua state the function lives in.
    /// @param type Where the body of the function lives.
    /// @param ref Handle on the function.
    /// @param name Name used in error messages.
    Function(lua_State* state, BindingType type, FunctionRef ref, std::string name = "unknown");

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
    /// Reads the arguments into a `CallContext`, runs the handler with it, then pushes the values of the returned
    /// pack onto the stack. Raises a Lua error if that Function was destroyed or has no handler, if an
    /// argument cannot be read, or if the handler, or pushing its values, throws an `exceptions::Base`. Every
    /// other `std::exception` is left to Luau, which turns it into a Lua runtime error, and anything that does not
    /// derive from `std::exception` raises a Lua error with a generic message.
    /// @param state The Lua state calling the closure.
    /// @return The number of values returned to Lua.
    /// @throws exceptions::InvalidOperation If the upvalue points to a Function wrapping a function defined in Luau.
    static int handle(lua_State* state);

    lua_State* _state;  ///< Lua state the function lives in.
    BindingType _type;  ///< Where the body of the wrapped function lives.
    FunctionRef _ref;   ///< Handle on the function.
    std::string _name;  ///< Name used in error messages.
    Handler _handler;   ///< C++ handler called by the native closure.
};
}  // namespace rtype::luau

#define RTYPE_FUNCTION_INCLUDING_TPP
#include "Function.tpp"  // IWYU pragma: export
#undef RTYPE_FUNCTION_INCLUDING_TPP
