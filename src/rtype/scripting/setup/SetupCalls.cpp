/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SetupCalls
*/

#include "SetupCalls.hpp"

#include <cstddef>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "core/asset/AssetId.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"
#include "rtype/scripting/call/EngineCall.hpp"

namespace rtype::scripting::setup {

SetupCalls::SetupCalls(const luau::Runtime& runtime, ecs::App& app) noexcept : _runtime{&runtime}, _app{&app} {}

bool SetupCalls::has(const std::string_view call) const {
    return _app->getWorld().getCalls().findByName(call) != nullptr;
}

luau::ValuePack SetupCalls::invoke(const std::string_view form, const std::string_view call,
                                   const CallFields& fields) const {
    ecs::World& world = _app->getWorld();
    const ecs::CallInfo* info = world.getCalls().findByName(call);
    if (info == nullptr) {
        throw luau::exceptions::RuntimeError{std::format("{}: unknown engine call '{}'", form, call)};
    }
    auto table = luau::Table::create(*_runtime, 0, static_cast<int>(fields.size()));
    if (!table) {
        throw luau::exceptions::RuntimeError{std::format("{}: {}", form, table.error().message())};
    }
    for (const auto& [name, value] : fields) {
        if (auto set = table->set(name, value); !set) {
            throw luau::exceptions::RuntimeError{std::format("{}: {}", form, set.error().message())};
        }
    }
    const luau::Value arguments{std::move(*table)};
    try {
        return call::callEngine(
            *_runtime, *info, &arguments,
            [&world, info](const std::span<const std::byte> bytes, const std::span<std::byte> result) {
                world.getCalls().invoke(world, info->id, bytes, result);
            });
    } catch (const luau::exceptions::RuntimeError& error) {
        throw luau::exceptions::RuntimeError{std::format("{}: {}", form, error.what())};
    }
}

luau::Value SetupCalls::assetId(const std::string_view path) const {
    auto userdata =
        _runtime->usertype<core::asset::AssetId>().create(core::asset::AssetId::fromPath(std::filesystem::path{path}));
    if (!userdata) {
        throw luau::exceptions::RuntimeError{std::string{userdata.error().message()}};
    }
    return luau::Value{std::move(*userdata)};
}
}  // namespace rtype::scripting::setup
