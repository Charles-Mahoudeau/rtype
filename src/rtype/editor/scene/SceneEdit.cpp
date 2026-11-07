/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SceneEdit
*/

#include "SceneEdit.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleFiles.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/ReloadedCapsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "plugins/capsule/format/CapsuleText.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::scene {

namespace {
namespace capsule = plugins::capsule;

/// @brief What a new capsule file holds: a root and nothing else.
constexpr std::string_view kEmptyCapsule = "[capsule format=1]\n\n[node path=\".\"]\n";

/// @return The node at @p index.
const capsule::CapsuleNode& nodeAt(const capsule::Capsule& capsule, std::size_t index) {
    return *std::next(capsule.getNodes().begin(), static_cast<std::ptrdiff_t>(index));
}

/// @brief Gives @p target @p source's side, instance and values, copied with their types' hooks.
void copyNode(capsule::CapsuleNode& target, const capsule::CapsuleNode& source, const ecs::Components& components) {
    target.side = source.side;
    target.instance = source.instance;
    for (std::size_t value = 0; value < source.values.getCount(); ++value) {
        target.values.set(components, source.values.getId(value), source.values.getValue(value));
    }
}
}  // namespace

bool isValidNodeName(std::string_view name) noexcept {
    return !name.empty() && name != "." && name != ".." && !name.contains('/');
}

std::vector<std::uint32_t> childrenOf(const capsule::Capsule& capsule, std::uint32_t node) {
    std::vector<std::uint32_t> children;
    const auto nodes = capsule.getNodes();
    for (std::size_t index = 1; index < nodes.size(); ++index) {
        if (nodeAt(capsule, index).parent == node) {
            children.push_back(static_cast<std::uint32_t>(index));
        }
    }
    return children;
}

bool isInSubtree(const capsule::Capsule& capsule, std::uint32_t node, std::uint32_t ancestor) {
    for (std::uint32_t current = node; current != capsule::kNoParent; current = nodeAt(capsule, current).parent) {
        if (current == ancestor) {
            return true;
        }
    }
    return false;
}

std::string uniqueChildName(const capsule::Capsule& capsule, std::uint32_t parent, std::string_view base) {
    const std::string stem = isValidNodeName(base) ? std::string{base} : std::string{"Node"};
    const std::vector<std::uint32_t> siblings = childrenOf(capsule, parent);
    const auto taken = [&](std::string_view name) {
        return std::ranges::any_of(siblings, [&](std::uint32_t child) { return nodeAt(capsule, child).name == name; });
    };
    if (!taken(stem)) {
        return stem;
    }
    for (std::size_t suffix = 2;; ++suffix) {
        std::string name = std::format("{}{}", stem, suffix);
        if (!taken(name)) {
            return name;
        }
    }
}

capsule::Capsule copyCapsule(const capsule::Capsule& source, const ecs::Components& components, const TreeEdit& edit) {
    if (edit.removed == 0U) {
        throw capsule::exceptions::CapsuleException{"The root of a capsule cannot be removed"};
    }
    if (edit.moved && (edit.moved->first == 0 || isInSubtree(source, edit.moved->second, edit.moved->first))) {
        throw capsule::exceptions::CapsuleException{"A node cannot move under itself, and the root cannot move"};
    }
    const std::size_t count = source.getNodes().size();
    std::vector<std::vector<std::uint32_t>> children(count);
    for (std::size_t index = 1; index < count; ++index) {
        const auto node = static_cast<std::uint32_t>(index);
        if (edit.removed == node || (edit.moved && edit.moved->first == node)) {
            continue;
        }
        children.at(nodeAt(source, index).parent).push_back(node);
    }
    if (edit.moved) {
        children.at(edit.moved->second).push_back(edit.moved->first);  // last among its new siblings
    }
    capsule::Capsule result{source.getPath()};
    copyNode(result.editNode(0), nodeAt(source, 0), components);
    // Depth first from the root: every parent is added before its children, as a capsule wants.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> pending;  // a source node and its new parent's index
    for (auto child = children.front().rbegin(); child != children.front().rend(); ++child) {
        pending.emplace_back(*child, 0U);
    }
    while (!pending.empty()) {
        const auto [node, parent] = pending.back();
        pending.pop_back();
        const capsule::CapsuleNode& original = nodeAt(source, node);
        const std::uint32_t added = result.addNode(uniqueChildName(result, parent, original.name), parent);
        copyNode(result.editNode(added), original, components);
        for (auto child = children.at(node).rbegin(); child != children.at(node).rend(); ++child) {
            pending.emplace_back(*child, added);
        }
    }
    return result;
}

std::uint32_t addChild(capsule::Capsule& capsule, std::uint32_t parent, std::string_view name,
                       std::optional<capsule::CapsuleRef> instance) {
    const std::uint32_t added = capsule.addNode(uniqueChildName(capsule, parent, name), parent);
    capsule.editNode(added).instance = std::move(instance);
    return added;
}

bool renameNode(capsule::Capsule& capsule, std::uint32_t node, std::string_view name) {
    if (node == 0 || node >= capsule.getNodes().size() || !isValidNodeName(name)) {
        return false;
    }
    capsule::CapsuleNode& renamed = capsule.editNode(node);
    if (renamed.name == name) {
        return true;
    }
    const std::vector<std::uint32_t> siblings = childrenOf(capsule, renamed.parent);
    if (std::ranges::any_of(siblings, [&](std::uint32_t child) { return nodeAt(capsule, child).name == name; })) {
        return false;
    }
    renamed.name = std::string{name};
    return true;
}

void commit(ecs::World& world, capsule::Capsule capsule) {
    const capsule::CapsuleId id = capsule.getId();
    world.getResources().requireMut<capsule::Capsules>()->define(std::move(capsule));
    respawn(world, id);
}

void respawn(ecs::World& world, capsule::CapsuleId id) {
    auto reloaded = world.getResources().requireMut<capsule::ReloadedCapsules>();
    if (std::ranges::find(reloaded->capsules, id) == reloaded->capsules.end()) {
        reloaded->capsules.push_back(id);
    }
}

std::optional<std::string> open(ecs::World& world, capsule::CapsuleId id) {
    ecs::Resources& resources = world.getResources();
    try {
        ecs::Commands commands{world};
        static_cast<void>(capsule::loadCapsule(commands, resources.require<capsule::Capsules>(),
                                               *resources.requireMut<capsule::LoadedCapsule>(), id));
        commands.flush();
    } catch (const capsule::exceptions::CapsuleException& error) {
        return std::string{error.what()};
    }
    return std::nullopt;
}

std::optional<std::string> save(const ecs::World& world, capsule::CapsuleId id, const std::filesystem::path& assets) {
    const auto& capsules = world.getResources().require<capsule::Capsules>();
    const capsule::Capsule* saved = capsules.find(id);
    if (saved == nullptr) {
        return std::string{"No capsule to save"};
    }
    const std::string text = capsule::format::writeCapsule(*saved, world.getComponents(), capsules);
    const std::filesystem::path file = assets / std::filesystem::path{std::string{saved->getPath()}};
    std::ofstream stream{file, std::ios::binary | std::ios::trunc};
    stream << text;
    if (!stream) {
        return std::format("Cannot write '{}'", file.string());
    }
    return std::nullopt;
}

std::optional<std::string> createCapsuleFile(ecs::World& world, const std::filesystem::path& assets,
                                             std::string_view path) {
    const std::string normalized = core::asset::normalizePath(std::filesystem::path{std::string{path}});
    if (normalized.empty() || !capsule::format::isCapsuleFile(normalized)) {
        return std::format("'{}' is not a capsule file: name it .scene, .actor or .capsule", path);
    }
    const std::filesystem::path file = assets / normalized;
    std::error_code error;
    if (std::filesystem::exists(file, error)) {
        return std::format("'{}' already exists", normalized);
    }
    std::filesystem::create_directories(file.parent_path(), error);
    std::ofstream stream{file, std::ios::binary};
    stream << kEmptyCapsule;
    stream.close();
    if (!stream) {
        return std::format("Cannot write '{}'", file.string());
    }
    const std::vector<std::string> errors = readFiles(world, std::span{&normalized, 1});
    return errors.empty() ? std::nullopt : std::optional{errors.front()};
}

std::vector<std::string> readFiles(ecs::World& world, std::span<const std::string> paths) {
    ecs::Resources& resources = world.getResources();
    auto files = resources.getMut<capsule::CapsuleFiles>();
    if (!files) {
        return {"The game has no asset directory"};
    }
    const capsule::CapsuleFiles::Reread reread = files->reload(*resources.requireMut<capsule::Capsules>(), paths);
    for (const capsule::CapsuleId id : reread.capsules) {
        respawn(world, id);
    }
    return reread.errors;
}

}  // namespace rtype::editor::scene
