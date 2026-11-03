/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptHost
*/

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "Export.hpp"
#include "component/ComponentLayout.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/luau/Library.hpp"
#include "rtype/luau/Result.hpp"
#include "rtype/luau/Runtime.hpp"

namespace rtype::scripting {
/// @brief Runs the scripts of a game on an App: owns their Luau runtime and the `engine` global they declare ECS data
/// through.
///
/// @details It knows no Plugin, so it can be used and tested with a bare App. `engine` holds the constant flags
/// `isClient`, `isServer` and `isDebug`, and the `ecs` library: `engine.ecs.component(name, {fields, defaults,
/// replication})` registers a component in the App's world (see readComponentDeclaration() for the table). The App
/// must outlive the host.
class RTYPE_SCRIPTING_API ScriptHost {
  public:
    /// @brief What the scripts can tell about the process they run in, as the constant flags of `engine`.
    struct Options {
        bool isClient{false};  ///< `engine.isClient`: this process runs the client side (also true when standalone).
        bool isServer{false};  ///< `engine.isServer`: this process runs the server side (also true when standalone).
        bool isDebug{false};   ///< `engine.isDebug`: this is a debug run.
    };

    /// @brief Creates the runtime and the `engine` global.
    /// @param app The App scripts declare their ECS data on; it must outlive the host.
    /// @param options The flags of `engine`.
    /// @return The host, or the error of the runtime that could not be created.
    [[nodiscard]] static luau::Result<std::unique_ptr<ScriptHost>> create(ecs::App& app, Options options);

    ~ScriptHost();
    ScriptHost(const ScriptHost& other) = delete;
    ScriptHost& operator=(const ScriptHost& other) = delete;
    ScriptHost(ScriptHost&& other) = delete;
    ScriptHost& operator=(ScriptHost&& other) = delete;

    /// @brief Compiles a script and runs it, once.
    /// @param name Name of the script, for the error messages.
    /// @param source Luau source code.
    /// @return Nothing, or the error of the script: it does not compile, or it raised one (a bad declaration is one).
    [[nodiscard]] luau::Result<> run(std::string name, const std::string& source) const;

    /// @param name Name of a component a script declared.
    /// @return Its layout, with the default value a field left out of `Commands::add` or `spawn` takes, or null if no
    /// script declared it. Valid while the host lives.
    [[nodiscard]] const component::ComponentLayout* findComponent(std::string_view name) const noexcept;

  private:
    /// @param app The App scripts declare their ECS data on.
    /// @param runtime The runtime of the scripts.
    ScriptHost(ecs::App& app, luau::Runtime runtime) noexcept;

    /// @brief Builds `engine` and its `ecs` library, and makes it a global.
    [[nodiscard]] luau::Result<> installEngine(Options options);

    /// @brief The body of `engine.ecs.component`.
    /// @throws luau::exceptions::RuntimeError When the declaration is wrong or the component cannot be registered.
    void declareComponent(std::string name, const luau::Table& spec);

    ecs::App& _app;          ///< The App scripts declare their ECS data on.
    luau::Runtime _runtime;  ///< The runtime of the scripts; declared before what holds references into it.
    std::optional<luau::Library> _engine;  ///< The `engine` library, with the sub-library `ecs`.
    std::unordered_map<std::string, component::ComponentLayout> _components;  ///< Components scripts declared.
};
}  // namespace rtype::scripting
