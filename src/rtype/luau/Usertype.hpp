/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Usertype
*/

#pragma once

#include <string>

#include "Result.hpp"
#include "Userdata.hpp"
#include "detail/UsertypeData.hpp"

namespace rtype::luau {
/// @brief Exposes a C++ type to Luau as userdata, with fields, methods and metamethods.
///
/// Obtained from `Runtime::usertype<T>()`, which registers the type once per runtime. A `Usertype` is a handle on that
/// registration: copies of it configure the same type, and members can be added at any time, even once userdata of
/// the type exist. The userdata holds a `T` by value, destroyed when Luau collects it or the runtime is closed.
///
/// Every handler is a callable. Its parameters are `Value::Variant` alternatives (`double` for numbers,
/// `std::string` for strings, `Table`...), or a `std::optional` of one for an optional trailing argument, and it takes
/// the object as `T&` or `const T&` first when it works on one. A call whose arguments do not fit raises a Luau
/// error, and so does an `exceptions::Base` thrown by a handler.
///
/// Scripts see members this way, in this order: getters, methods, then `index()`; writing goes to setters, then
/// `newIndex()`.
/// @tparam T The C++ type. It must be destructible and aligned for a `double` at most.
template <typename T>
class Usertype {
  public:
    /// @brief Creates the handle of a registered type. Use `Runtime::usertype()` instead.
    /// @param data The registration of the type, which must outlive the handle.
    explicit Usertype(detail::UsertypeData& data) noexcept;
    ~Usertype() = default;
    Usertype(const Usertype&) = default;
    Usertype& operator=(const Usertype&) = default;
    Usertype(Usertype&&) noexcept = default;
    Usertype& operator=(Usertype&&) noexcept = default;

    /// @brief Gets the name of the type for scripts, which `typeof` gives for its userdata.
    /// @return The name.
    [[nodiscard]] const std::string& name() const noexcept;

    /// @brief Lets scripts create objects: `Name.new(...)`, a function of a global table `Name`.
    /// @tparam F Callable `T(Args...)`.
    /// @param function Builds the object from the arguments of the script.
    /// @return This type.
    /// @throws exceptions::InvalidOperation If a global of the name of the type already exists.
    template <typename F>
    Usertype& constructor(F&& function);

    /// @brief Adds a readable member: scripts read `object.name`.
    /// @tparam F Callable `R(const T&)`, where a `Value` can be built from `R`.
    /// @param name Name of the member.
    /// @param function Reads the member.
    /// @return This type.
    template <typename F>
    Usertype& getter(std::string name, F&& function);

    /// @brief Adds a writable member: scripts write `object.name = value`.
    /// @tparam F Callable `void(T&, V)`, where `V` is the type the script must give.
    /// @param name Name of the member.
    /// @param function Writes the member.
    /// @return This type.
    template <typename F>
    Usertype& setter(std::string name, F&& function);

    /// @brief Adds a method: scripts call `object:name(...)`.
    /// @tparam F Callable `R(T&, Args...)` (or `const T&`), where `R` is `void`, a `ValuePack`, or a type a `Value`
    ///         can be built from.
    /// @param name Name of the method.
    /// @param function What the method does.
    /// @return This type.
    /// @throws exceptions::InvalidOperation If `name` contains a NUL character.
    template <typename F>
    Usertype& method(std::string name, F&& function);

    /// @brief Handles the reads of members that have no getter nor method, such as fields known at runtime.
    ///
    /// Without it, reading such a member raises a Luau error.
    /// @tparam F Callable convertible to `std::function<Value(T&, std::string_view)>`; it raises an
    ///         `exceptions::RuntimeError` for a name it does not know.
    /// @param function Reads the member of the given name.
    /// @return This type.
    template <typename F>
    Usertype& index(F&& function);

    /// @brief Handles the writes of members that have no setter.
    ///
    /// Without it, writing such a member raises a Luau error.
    /// @tparam F Callable convertible to `std::function<void(T&, std::string_view, const Value&)>`.
    /// @param function Writes the member of the given name.
    /// @return This type.
    template <typename F>
    Usertype& newIndex(F&& function);

    /// @brief Sets `__eq`: two userdata of the type are equal for `==` when @p function says so.
    /// @tparam F Callable convertible to `std::function<bool(const T&, const T&)>`.
    /// @param function Compares two objects.
    /// @return This type.
    template <typename F>
    Usertype& eq(F&& function);

    /// @brief Sets `__tostring`.
    /// @tparam F Callable convertible to `std::function<std::string(const T&)>`.
    /// @param function Gives the text of an object.
    /// @return This type.
    template <typename F>
    Usertype& tostring(F&& function);

    /// @brief Creates a userdata holding an object, to hand to scripts.
    /// @param value The object, moved into the userdata.
    /// @return A handle on the userdata, or a Failure of kind `kOutOfMemory` if Luau could not allocate it, or of kind
    ///         `kRuntime` if the object could not be constructed.
    [[nodiscard]] Result<Userdata> create(T value) const;

  private:
    detail::UsertypeData* _data;  ///< The registration of the type, owned by the runtime.
};
}  // namespace rtype::luau

#define RTYPE_USERTYPE_INCLUDING_TPP
#include "Usertype.tpp"  // IWYU pragma: export
#undef RTYPE_USERTYPE_INCLUDING_TPP
