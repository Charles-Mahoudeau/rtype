/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SetupCalls
*/

#pragma once

#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace rtype::scripting::setup {
/// @brief The argument fields of an engine call a form makes, by name: `{"path", "ship.png"}, {"linear", true}`.
using CallFields = std::vector<std::pair<std::string_view, luau::Value>>;

/// @brief What the setup forms make their engine calls with: the App the scripts set up, and their runtime.
///
/// @details A form calls the engine calls a plugin registered (ecs::Calls), named as the plugin names them. It checks
/// has() first: a plugin that is not in the App, such as the sprite plugin on a dedicated server, registered none, and
/// the form then skips its calls.
class SetupCalls {
  public:
    /// @param runtime The scripts' runtime, which builds the values of the results; it must outlive the forms.
    /// @param app The App the scripts set up; it must outlive the forms' calls.
    SetupCalls(const luau::Runtime& runtime, ecs::App& app) noexcept;
    ~SetupCalls() = default;
    SetupCalls(const SetupCalls&) = default;
    SetupCalls& operator=(const SetupCalls&) = default;
    SetupCalls(SetupCalls&&) noexcept = default;
    SetupCalls& operator=(SetupCalls&&) noexcept = default;

    /// @return Whether the App has the engine call @p call.
    [[nodiscard]] bool has(std::string_view call) const;

    /// @brief Calls an engine call with argument fields, as `engine.call` does.
    /// @param form The form making the call, the start of its error messages.
    /// @param call The call's name.
    /// @param fields Its argument fields.
    /// @return Its result's values (see call::callEngine()).
    /// @throws luau::exceptions::RuntimeError `<form>: <what>`, when the call is unknown, a field is wrong or the call
    /// throws.
    [[nodiscard]] luau::ValuePack invoke(std::string_view form, std::string_view call, const CallFields& fields) const;

    /// @return The `AssetId` of the asset at @p path (core::asset::AssetId::fromPath()), as a Luau value.
    /// @throws luau::exceptions::RuntimeError If the value cannot be created.
    [[nodiscard]] luau::Value assetId(std::string_view path) const;

  private:
    const luau::Runtime* _runtime;  ///< The scripts' runtime.
    ecs::App* _app;                 ///< The App the scripts set up.
};
}  // namespace rtype::scripting::setup
