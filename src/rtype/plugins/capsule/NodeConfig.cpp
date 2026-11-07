/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NodeConfig
*/

#include "NodeConfig.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "core/asset/AssetFiles.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::plugins::capsule {
namespace {
/// @return The world's capsules.
/// @throws exceptions::CapsuleException When the world has none: the CapsulePlugin was not added.
Capsules& requireCapsules(ecs::World& world) {
    ecs::Mut<Capsules> capsules = world.getResources().getMut<Capsules>();
    if (!capsules) {
        throw exceptions::CapsuleException{
            std::format("The world has no capsules: add the CapsulePlugin ('{}') first", CapsulePlugin::kName)};
    }
    return *capsules;
}
}  // namespace

NodeConfig::NodeConfig(ecs::World& world, CapsuleId capsule, std::uint32_t node) noexcept
    : _world{&world}, _capsule{capsule}, _node{node} {}

CapsuleId NodeConfig::getCapsule() const noexcept { return _capsule; }

std::uint32_t NodeConfig::getNode() const noexcept { return _node; }

NodeConfig& NodeConfig::with(ecs::ComponentId id, std::span<const std::byte> value) {
    editNode().values.set(_world->getComponents(), id, value);
    return *this;
}

NodeConfig NodeConfig::child(std::string name) {
    return NodeConfig{*_world, _capsule, getCapsules().edit(_capsule).addNode(std::move(name), _node)};
}

NodeConfig& NodeConfig::instance(std::string_view path) {
    std::string normalized = core::asset::normalizePath(path);
    const CapsuleId id = CapsuleId::fromPath(normalized);
    editNode().instance = CapsuleRef{.id = id, .path = std::move(normalized)};
    return *this;
}

NodeConfig& NodeConfig::onSide(ecs::Side side) {
    if (_node == 0) {
        throw exceptions::CapsuleException{
            std::format("Capsule '{}': the root is spawned on every side", getCapsules().edit(_capsule).getPath())};
    }
    editNode().side = side;
    return *this;
}

CapsuleNode& NodeConfig::editNode() const { return getCapsules().edit(_capsule).editNode(_node); }

Capsules& NodeConfig::getCapsules() const { return requireCapsules(*_world); }

NodeConfig addCapsule(ecs::App& app, std::string_view path) {
    ecs::World& world = app.getWorld();
    return NodeConfig{world, requireCapsules(world).define(path).getId(), 0};
}
}  // namespace rtype::plugins::capsule
