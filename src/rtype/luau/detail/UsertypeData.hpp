/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UsertypeData
*/

#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

#include "rtype/luau/Export.hpp"
#include "rtype/luau/MapHelper.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

struct lua_State;

namespace rtype::luau::detail {
/// @brief What a usertype is for one Lua state, without the C++ type: its tag, its metatable and the handlers behind
/// it.
///
/// The typed side (`Usertype<T>`) wraps the handlers of `T` into the erased `std::function`s below and gives them to
/// this class, which owns them and builds the metatable of the userdata: `__index` and `__newindex` look the member
/// up in the handlers when a script uses it, so members can be added at any time. It holds no reference to the Lua
/// state (no `Ref`), only the handlers.
///
/// Handlers throw `exceptions::Base` to report a misuse: the metamethods turn it into a Luau error.
class RTYPE_LUAU_API UsertypeData {
  public:
    /// @brief Reads a member of an object.
    using Getter = std::function<Value(void* self)>;
    /// @brief Writes a member of an object.
    using Setter = std::function<void(void* self, const Value& value)>;
    /// @brief Calls a method of an object; @p args does not include the object.
    using Method = std::function<ValuePack(void* self, std::span<const Value> args)>;
    /// @brief Constructs an object in the given storage from the arguments of a script.
    using Constructor = std::function<void(void* storage, std::span<const Value> args)>;
    /// @brief Reads a member that has no getter nor method.
    using IndexFallback = std::function<Value(void* self, std::string_view key)>;
    /// @brief Writes a member that has no setter.
    using NewIndexFallback = std::function<void(void* self, std::string_view key, const Value& value)>;
    /// @brief Compares two objects.
    using Equality = std::function<bool(const void* lhs, const void* rhs)>;
    /// @brief Converts an object to a string.
    using Stringifier = std::function<std::string(const void* self)>;
    /// @brief Destroys the object held by a userdata, when the Lua state collects it or is closed.
    using Destructor = void (*)(lua_State* state, void* object);

    /// @brief Registers a usertype in a Lua state: sets the destructor of its tag and builds its metatable.
    /// @param state The main thread of the Lua state; must not be null.
    /// @param name Name of the type; it is what `typeof` gives for its userdata.
    /// @param tag Userdata tag of the type, unique in the state.
    /// @param size Size in bytes of the object held by a userdata.
    /// @param destructor Destroys an object; runs on a tagged userdata only.
    UsertypeData(lua_State* state, std::string name, int tag, std::size_t size, Destructor destructor);
    ~UsertypeData() = default;
    UsertypeData(const UsertypeData&) = delete;
    UsertypeData& operator=(const UsertypeData&) = delete;
    UsertypeData(UsertypeData&&) noexcept = delete;
    UsertypeData& operator=(UsertypeData&&) noexcept = delete;

    /// @brief Gets the name of the type.
    /// @return The name.
    [[nodiscard]] const std::string& name() const noexcept;
    /// @brief Gets the userdata tag of the type.
    /// @return The tag.
    [[nodiscard]] int tag() const noexcept;

    /// @brief Constructs the object of a new userdata with a function pointer, so that no allocation is needed.
    /// @param storage The storage of the object, to construct it in.
    /// @param source What the object is made from.
    using Initializer = void (*)(void* storage, void* source);

    /// @brief Creates a userdata for an object, and hands it to C++ as a handle.
    ///
    /// Leaves the stack untouched. If @p initialize throws, nothing is left behind and the destructor does not run.
    /// @param initialize Constructs the object in the storage of the userdata.
    /// @param source Given as is to @p initialize.
    /// @return A handle on the userdata, or a Failure of kind `kRuntime` if @p initialize threw or Luau could not
    ///         allocate the userdata.
    [[nodiscard]] Result<Userdata> create(Initializer initialize, void* source) const;

    /// @brief Gets the object of the userdata at a stack index.
    /// @param state The Lua thread to read from.
    /// @param index Stack index of the userdata.
    /// @return The object, or `nullptr` if the value is not a userdata of this type.
    [[nodiscard]] void* objectAt(lua_State* state, int index) const;

    /// @brief Adds (or replaces) a getter: scripts read `object.name`.
    void addGetter(std::string name, Getter getter);
    /// @brief Adds (or replaces) a setter: scripts write `object.name = value`.
    void addSetter(std::string name, Setter setter);
    /// @brief Adds a method: scripts call `object:name(...)`. A getter of the same name takes precedence.
    void addMethod(std::string name, Method method);
    /// @brief Sets the constructor, and exposes it to scripts as `<type name>.new(...)`, a function of a global table.
    /// @throws exceptions::InvalidOperation If a global of that name already exists (and is not this type's table).
    void setConstructor(Constructor constructor);
    /// @brief Sets the handler of the members that have no getter nor method; without it, they raise an error.
    void setIndexFallback(IndexFallback fallback);
    /// @brief Sets the handler of the members that have no setter; without it, writing them raises an error.
    void setNewIndexFallback(NewIndexFallback fallback);
    /// @brief Sets `__eq`: two userdata of the type compare equal when it says so.
    void setEquality(Equality equality);
    /// @brief Sets `__tostring`.
    void setStringifier(Stringifier stringifier);

  private:
    /// @brief A method with its name, kept at a stable address because the closure of the method points to it.
    struct MethodEntry {
        std::string name;  ///< Name of the method.
        Method handler;    ///< What the method does.
    };

    /// @brief The members of a map by name, looked up by `std::string_view` without building a string.
    template <typename T>
    using MemberMap = std::unordered_map<std::string, T, StringHash, std::equal_to<>>;

    /// @name C closures of the metatable
    /// Each one has the `UsertypeData` as first upvalue and raises a Luau error for any `exceptions::Base`.
    /// @{
    static int index(lua_State* state);
    static int newIndex(lua_State* state);
    static int callMethod(lua_State* state);
    static int construct(lua_State* state);
    static int equal(lua_State* state);
    static int stringify(lua_State* state);
    /// @}

    /// @brief Pushes a new userdata, big enough for an object but not yet usable by scripts.
    ///
    /// The userdata has no tag and no metatable until `seal()`, so nothing is destroyed if constructing the object
    /// throws. The caller constructs the object in the storage returned, then calls `seal()`.
    /// @param state The Lua thread to push onto.
    /// @return The storage for the object.
    [[nodiscard]] void* allocate(lua_State* state) const;
    /// @brief Gives the userdata on top of the stack its tag and its metatable, once its object is constructed.
    /// @param state The Lua thread whose top is a userdata pushed by `allocate()`.
    void seal(lua_State* state) const;

    /// @brief Pushes the metatable of the type.
    void pushMetatable() const;

    lua_State* _state;                   ///< Main thread of the Lua state the type is registered in.
    std::string _name;                   ///< Name of the type.
    int _tag;                            ///< Userdata tag of the type.
    std::size_t _size;                   ///< Size in bytes of the object held by a userdata.
    MemberMap<Getter> _getters;          ///< Readable members.
    MemberMap<Setter> _setters;          ///< Writable members.
    std::deque<MethodEntry> _methods;    ///< Methods; a deque keeps the entries the closures point to in place.
    Constructor _constructor;            ///< Builds an object from the arguments of `<name>.new`.
    IndexFallback _indexFallback;        ///< Reads the members that have no getter nor method.
    NewIndexFallback _newIndexFallback;  ///< Writes the members that have no setter.
    Equality _equality;                  ///< `__eq`.
    Stringifier _stringifier;            ///< `__tostring`.
};
}  // namespace rtype::luau::detail
