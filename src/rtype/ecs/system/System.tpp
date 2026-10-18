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

template <typename... Params>
FunctionSystem<Params...>::FunctionSystem(World& world, Function function, SystemAccess& access)
    : _function{function}, _states{SystemParam<Params>::init(world)...} {
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

template <typename... Params>
System::System(World& world, std::string name, void (*function)(Params...))
    : _world{&world},
      _name{std::move(name)},
      _commands{world},
      _lastRun{world.getChangeTick() - kMaxChangeAge},  // the first run sees everything as added
      _body{makeBody(world, function, _access)} {}

template <typename Function>
System::System(World& world, std::string name, Function function)
    : System{world, std::move(name), toPointer(function)} {}

template <typename... Params>
std::unique_ptr<detail::SystemBody> System::makeBody(World& world, void (*function)(Params...), SystemAccess& access) {
    if constexpr ((SystemParameter<Params> && ...)) {
        return std::make_unique<detail::FunctionSystem<Params...>>(world, function, access);
    } else {
        static_assert(detail::kDependentFalse<void (*)(Params...)>,
                      "A system parameter must be one of: Query<Terms...>, Res<T>, ResMut<T>, NonSend<T>, "
                      "NonSendMut<T>, EventReader<T>, EventWriter<T> (taken by value), or Commands& (by reference).");
        return nullptr;
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
