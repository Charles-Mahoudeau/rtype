/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FunctionCall
*/

#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <tuple>
#include <utility>

#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
namespace detail {
/// @brief The body of an engine call made of a C++ function: the function and the state of each parameter.
/// @tparam Function The function pointer.
/// @tparam Arguments Its call data argument, or void when it takes none.
/// @tparam Result Its call data result, or void when it returns none.
/// @tparam ParamList A std::tuple of its system parameters, in order.
template <typename Function, typename Arguments, typename Result, typename ParamList>
class FunctionCall;

/// @brief The body of an engine call whose system parameters are `Params...`.
template <typename Function, typename Arguments, typename Result, typename... Params>
class FunctionCall<Function, Arguments, Result, std::tuple<Params...>> final : public CallBody {
  public:
    /// @brief Builds each parameter's state, registering what it needs.
    FunctionCall(World& world, Function function);
    ~FunctionCall() override = default;
    FunctionCall(const FunctionCall& other) = delete;
    FunctionCall& operator=(const FunctionCall& other) = delete;
    FunctionCall(FunctionCall&& other) = delete;
    FunctionCall& operator=(FunctionCall&& other) = delete;

    [[nodiscard]] std::unique_ptr<CallBody> bind(World& world) const override;

    void addAccess(SystemAccess& access) const override;

    void invoke(const SystemContext& context, std::span<const std::byte> arguments,
                std::span<std::byte> result) override;

  private:
    /// @brief Adds the access of every parameter.
    template <std::size_t... Indices>
    void addAccess(SystemAccess& access, std::index_sequence<Indices...> indices) const;

    /// @brief Calls the function with every parameter built for this call, and the arguments if it takes some.
    template <std::size_t... Indices>
    void call(const SystemContext& context, std::span<const std::byte> arguments, std::span<std::byte> result,
              std::index_sequence<Indices...> indices);

    Function _function;                                          ///< The call's function.
    std::tuple<typename SystemParam<Params>::State...> _states;  ///< What each parameter keeps between calls.
};
}  // namespace detail

/// @brief Describes an engine call made of a C++ function, ready for Calls::add().
///
/// @details The function is a function or a lambda without captures, as a system is. Its parameters are, in order:
/// system parameters (`Res<T>`, `ResMut<T>`, `NonSend<T>`, `NonSendMut<T>`, `Query<Terms...>`, `Commands&`,
/// `EventWriter<T>`, `EventReader<T>`, but no configured parameter), which give the call's access as they give a
/// system's; then, optionally, its arguments, `const Args&` where `Args` is declared with RTYPE_ECS_CALL_DATA. It
/// returns void or a type declared with RTYPE_ECS_CALL_DATA. Anything else fails to compile, with a message.
/// @code
/// struct LoadSound {
///     rtype::core::asset::AssetId path;
/// };
/// RTYPE_ECS_CALL_DATA(LoadSound, "rtype.audio.LoadSound", path);
/// struct Loaded {
///     rtype::core::asset::AssetId id;
/// };
/// RTYPE_ECS_CALL_DATA(Loaded, "rtype.audio.Loaded", id);
///
/// app.addCall("rtype.audio.loadSound", [](rtype::ecs::ResMut<AudioAssets> sounds, const LoadSound& args) {
///     return Loaded{.id = sounds->load(args.path)};
/// });
/// @endcode
/// @param world The world it is registered in: its parameters register what they need there.
/// @param name Its stable name, such as "rtype.audio.loadSound".
/// @param function The function.
/// @return The call's description, with its body and its access; the id is left for Calls::add().
template <typename Function>
[[nodiscard]] CallInfo makeCallInfo(World& world, std::string name, Function function);
}  // namespace rtype::ecs

#define RTYPE_ECS_FUNCTION_CALL_INCLUDING_TPP
#include "FunctionCall.tpp"  // IWYU pragma: export
#undef RTYPE_ECS_FUNCTION_CALL_INCLUDING_TPP
