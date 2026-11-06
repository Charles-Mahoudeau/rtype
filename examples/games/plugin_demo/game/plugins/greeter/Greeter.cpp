/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Greeter
*/

// A game's own plugin: this folder, its xmake.lua and one line in project.luau are all it takes. No file of the engine
// changes for it. Scripts call it with `engine.call("duel.greet", { name = "pilot" })`.

#include <format>
#include <memory>
#include <string>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/type/String.hpp"

namespace {
/// @brief The arguments of `duel.greet`.
struct GreetArguments {
    rtype::ecs::String name;  ///< Who to greet.
};
RTYPE_ECS_CALL_DATA(GreetArguments, "duel.GreetArguments", name);

/// @brief The result of `duel.greet`.
struct GreetResult {
    rtype::ecs::String message;  ///< The greeting.
};
RTYPE_ECS_CALL_DATA(GreetResult, "duel.GreetResult", message);

GreetResult greet(const GreetArguments& arguments) {
    return GreetResult{.message = rtype::ecs::String{std::format("Good luck, {}!", arguments.name.view())}};
}

/// @brief Adds the call `duel.greet`.
class GreeterPlugin final : public rtype::ecs::Plugin {
  public:
    static constexpr std::string_view kName = "duel.Greeter";  ///< The plugin's name, as project.luau names it.

    GreeterPlugin() = default;
    ~GreeterPlugin() override = default;
    GreeterPlugin(const GreeterPlugin& other) = default;
    GreeterPlugin& operator=(const GreeterPlugin& other) = default;
    GreeterPlugin(GreeterPlugin&& other) noexcept = default;
    GreeterPlugin& operator=(GreeterPlugin&& other) noexcept = default;

    [[nodiscard]] std::string_view getName() const noexcept override { return kName; }
    void build(rtype::ecs::App& app) override { app.addCall("duel.greet", &greet); }
};
}  // namespace

RTYPE_PLUGIN_LIBRARY(registrar) {
    registrar.getPlugins().add(std::string{GreeterPlugin::kName}, [] { return std::make_unique<GreeterPlugin>(); });
}
