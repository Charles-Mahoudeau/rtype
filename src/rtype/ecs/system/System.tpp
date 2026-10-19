/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** System
*/

#include <cstddef>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace rtype::ecs {
namespace detail {
/// @brief False for every type, so that a static_assert using it only fires when its template is instantiated.
template <typename T>
inline constexpr bool kDependentFalse = false;

/// @brief How many of `Types` are `T`.
template <typename T, typename... Types>
inline constexpr std::size_t kCountOf = (static_cast<std::size_t>(std::is_same_v<T, Types>) + ... + 0);

/// @brief The configuration type of a parameter: `SystemParam<Param>::Config`, or void when it takes none.
template <typename Param>
struct ConfigOf {
    using Type = void;  ///< Not configured.
};

/// @brief The configuration type of a configured parameter.
template <ConfiguredParameter Param>
struct ConfigOf<Param> {
    using Type = SystemParam<Param>::Config;  ///< What its init() takes.
};

/// @brief Whether the configurations given to a system match its configured parameters.
template <typename ParamList, typename ConfigList>
struct ConfigsMatch;

/// @brief Whether `Configs` holds, exactly once, the configuration of each configured parameter of `Params`, and
/// nothing else.
template <typename... Params, typename... Configs>
struct ConfigsMatch<std::tuple<Params...>, std::tuple<Configs...>> {
    /// @brief Every configured parameter finds its configuration exactly once.
    static constexpr bool kEachParamConfigured =
        ((!ConfiguredParameter<Params> || kCountOf<typename ConfigOf<Params>::Type, Configs...> == 1) && ...);

    /// @brief Every configuration is the configuration of exactly one parameter.
    static constexpr bool kEachConfigUsed = ((kCountOf<Configs, typename ConfigOf<Params>::Type...> == 1) && ...);
};

/// @return The state of `Param`: built from its configuration (moved out of `configs`) when it is configured.
template <typename Param, typename... Configs>
[[nodiscard]] typename SystemParam<Param>::State initState(World& world, std::tuple<Configs...>& configs) {
    if constexpr (ConfiguredParameter<Param>) {
        return SystemParam<Param>::init(world, std::move(std::get<typename SystemParam<Param>::Config>(configs)));
    } else {
        return SystemParam<Param>::init(world);
    }
}

template <typename Callable, typename... Params>
template <typename... Configs>
FunctionSystem<Callable, Params...>::FunctionSystem(World& world, Callable function, SystemAccess& access,
                                                    std::tuple<Configs...>& configs)
    : _function{std::move(function)}, _states{initState<Params>(world, configs)...} {
    addAccess(access, std::index_sequence_for<Params...>{});
}

template <typename Callable, typename... Params>
void FunctionSystem<Callable, Params...>::run(const SystemContext& context) {
    call(context, std::index_sequence_for<Params...>{});
}

template <typename Callable, typename... Params>
template <std::size_t... Indices>
void FunctionSystem<Callable, Params...>::addAccess(SystemAccess& access,
                                                    std::index_sequence<Indices...> /*indices*/) const {
    (SystemParam<Params>::addAccess(std::get<Indices>(_states), access), ...);
}

template <typename Callable, typename... Params>
template <std::size_t... Indices>
void FunctionSystem<Callable, Params...>::call(const SystemContext& context,
                                               std::index_sequence<Indices...> /*indices*/) {
    _function(SystemParam<Params>::get(std::get<Indices>(_states), context)...);
}
/// @brief The parameters and return type of a callable's call operator.
template <typename Method>
struct CallSignature;

/// @brief A const call operator (a lambda).
template <typename Class, typename Return, typename... Args>
struct CallSignature<Return (Class::*)(Args...) const> {
    using Type = std::type_identity<void(Args...)>;  ///< The parameters, as a system's signature.
    using Result = Return;                           ///< What it returns.
};

/// @brief A non-const call operator (a mutable lambda).
template <typename Class, typename Return, typename... Args>
struct CallSignature<Return (Class::*)(Args...)> {
    using Type = std::type_identity<void(Args...)>;  ///< The parameters, as a system's signature.
    using Result = Return;                           ///< What it returns.
};

/// @brief A const noexcept call operator.
template <typename Class, typename Return, typename... Args>
struct CallSignature<Return (Class::*)(Args...) const noexcept> {
    using Type = std::type_identity<void(Args...)>;  ///< The parameters, as a system's signature.
    using Result = Return;                           ///< What it returns.
};

/// @brief A non-const noexcept call operator.
template <typename Class, typename Return, typename... Args>
struct CallSignature<Return (Class::*)(Args...) noexcept> {
    using Type = std::type_identity<void(Args...)>;  ///< The parameters, as a system's signature.
    using Result = Return;                           ///< What it returns.
};
}  // namespace detail

template <typename... Params, typename... Configs>
System::System(World& world, std::string name, void (*function)(Params...), Configs... configs)
    : _world{&world},
      _name{std::move(name)},
      _commands{world},
      _lastRun{world.getChangeTick() - kMaxChangeAge},  // the first run sees everything as added
      _body{makeBody(world, function, std::type_identity<void(Params...)>{}, _access, std::move(configs)...)} {}

template <typename Function, typename... Configs>
System::System(World& world, std::string name, Function function, Configs... configs)
    : System{world, std::move(name), toPointer(function), std::move(configs)...} {}

template <typename Callable, typename... Configs>
System::System(World& world, std::string name, ScriptCallable<Callable> script, Configs... configs)
    : _world{&world},
      _name{std::move(name)},
      _commands{world},
      _lastRun{world.getChangeTick() - kMaxChangeAge},  // the first run sees everything as added
      _body{makeScriptBody(world, std::move(script.callable), _access, std::move(configs)...)} {}

template <typename Callable, typename... Params, typename... Configs>
std::unique_ptr<detail::SystemBody> System::makeBody(World& world, Callable function,
                                                     std::type_identity<void(Params...)> /*signature*/,
                                                     SystemAccess& access, Configs... configs) {
    using Match = detail::ConfigsMatch<std::tuple<Params...>, std::tuple<Configs...>>;
    if constexpr (!(SystemParameter<Params> && ...)) {
        static_assert(detail::kDependentFalse<void (*)(Params...)>,
                      "A system parameter must be one of: Query<Terms...>, Res<T>, ResMut<T>, NonSend<T>, "
                      "NonSendMut<T>, EventReader<T>, EventWriter<T>, a configured parameter (taken by value), or "
                      "Commands& (by reference).");
        return nullptr;
    } else if constexpr (!Match::kEachParamConfigured) {
        static_assert(detail::kDependentFalse<void (*)(Params...)>,
                      "A configured system parameter needs its configuration (the Config type of its SystemParam), "
                      "given once after the function when the system is added.");
        return nullptr;
    } else if constexpr (!Match::kEachConfigUsed) {
        static_assert(detail::kDependentFalse<void (*)(Params...)>,
                      "Each configuration given after the function must be the configuration of exactly one parameter "
                      "of the system.");
        return nullptr;
    } else {
        std::tuple<Configs...> configList{std::move(configs)...};
        return std::make_unique<detail::FunctionSystem<Callable, Params...>>(world, std::move(function), access,
                                                                             configList);
    }
}

template <typename Callable, typename... Configs>
std::unique_ptr<detail::SystemBody> System::makeScriptBody(World& world, Callable callable, SystemAccess& access,
                                                           Configs... configs) {
    if constexpr (!requires { &Callable::operator(); }) {
        static_assert(detail::kDependentFalse<Callable>,
                      "A ScriptCallable needs a callable with one call operator and explicit parameter types (no "
                      "`auto` parameters).");
        return nullptr;
    } else {
        using Signature = detail::CallSignature<decltype(&Callable::operator())>;
        if constexpr (!std::is_void_v<typename Signature::Result>) {
            static_assert(detail::kDependentFalse<Callable>, "A ScriptCallable must return void, as a system does.");
            return nullptr;
        } else {
            return makeBody(world, std::move(callable), typename Signature::Type{}, access, std::move(configs)...);
        }
    }
}

template <typename Function>
auto System::toPointer(Function function) {
    if constexpr (requires { requires std::is_pointer_v<decltype(+function)>; }) {
        return +function;  // a lambda without captures converts to a function pointer
    } else {
        static_assert(detail::kDependentFalse<Function>,
                      "A system must be a function or a lambda without captures (and without `auto` parameters): a "
                      "system keeps no hidden state, put it in a component or a resource. A script runtime passes its "
                      "callable through ScriptCallable.");
        return static_cast<void (*)()>(nullptr);
    }
}
}  // namespace rtype::ecs
