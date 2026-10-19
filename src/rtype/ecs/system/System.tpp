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

template <typename... Params>
template <typename... Configs>
FunctionSystem<Params...>::FunctionSystem(World& world, Function function, SystemAccess& access,
                                          std::tuple<Configs...>& configs)
    : _function{function}, _states{initState<Params>(world, configs)...} {
    addAccess(access, std::index_sequence_for<Params...>{});
}

template <typename... Params>
void FunctionSystem<Params...>::run(const SystemContext& context) {
    call(context, std::index_sequence_for<Params...>{});
}

template <typename... Params>
template <std::size_t... Indices>
void FunctionSystem<Params...>::addAccess(SystemAccess& access, std::index_sequence<Indices...> /*indices*/) const {
    (SystemParam<Params>::addAccess(std::get<Indices>(_states), access), ...);
}

template <typename... Params>
template <std::size_t... Indices>
void FunctionSystem<Params...>::call(const SystemContext& context, std::index_sequence<Indices...> /*indices*/) {
    _function(SystemParam<Params>::get(std::get<Indices>(_states), context)...);
}
}  // namespace detail

template <typename... Params, typename... Configs>
System::System(World& world, std::string name, void (*function)(Params...), Configs... configs)
    : _world{&world},
      _name{std::move(name)},
      _commands{world},
      _lastRun{world.getChangeTick() - kMaxChangeAge},  // the first run sees everything as added
      _body{makeBody(world, function, _access, std::move(configs)...)} {}

template <typename Function, typename... Configs>
System::System(World& world, std::string name, Function function, Configs... configs)
    : System{world, std::move(name), toPointer(function), std::move(configs)...} {}

template <typename... Params, typename... Configs>
std::unique_ptr<detail::SystemBody> System::makeBody(World& world, void (*function)(Params...), SystemAccess& access,
                                                     Configs... configs) {
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
        return std::make_unique<detail::FunctionSystem<Params...>>(world, function, access, configList);
    }
}

template <typename Function>
auto System::toPointer(Function function) {
    if constexpr (requires { requires std::is_pointer_v<decltype(+function)>; }) {
        return +function;  // a lambda without captures converts to a function pointer
    } else {
        static_assert(detail::kDependentFalse<Function>,
                      "A system must be a function or a lambda without captures (and without `auto` parameters): a "
                      "system keeps no hidden state, put it in a component or a resource.");
        return static_cast<void (*)()>(nullptr);
    }
}
}  // namespace rtype::ecs
