/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Capsule
*/

#include "Capsule.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/asset/AssetFiles.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"

namespace rtype::plugins::capsule {
Capsule::Capsule(std::string_view path) : _path{core::asset::normalizePath(path)}, _nodes(1) {
    if (path.empty()) {
        throw exceptions::CapsuleException{"A capsule needs a path"};
    }
    _id = CapsuleId::fromPath(_path);
}

CapsuleId Capsule::getId() const noexcept { return _id; }

std::string_view Capsule::getPath() const noexcept { return _path; }

std::uint32_t Capsule::getVersion() const noexcept { return _version; }

std::span<const CapsuleNode> Capsule::getNodes() const noexcept { return _nodes; }

CapsuleNode& Capsule::editNode(std::size_t index) {
    if (index >= _nodes.size()) {
        throw exceptions::CapsuleException{
            std::format("Capsule '{}' has {} node(s), not a node {}", _path, _nodes.size(), index)};
    }
    return *std::next(_nodes.begin(), static_cast<std::ptrdiff_t>(index));
}

std::uint32_t Capsule::addNode(std::string name, std::uint32_t parent) {
    if (parent >= _nodes.size()) {
        throw exceptions::CapsuleException{
            std::format("Capsule '{}' has no node {} to add '{}' under", _path, parent, name)};
    }
    if (name.empty() || name == "." || name == ".." || name.contains('/')) {
        throw exceptions::CapsuleException{std::format(
            "Capsule '{}': '{}' cannot name a node (not empty, without '/', not '.' nor '..')", _path, name)};
    }
    const bool taken = std::ranges::any_of(
        _nodes, [&](const CapsuleNode& node) { return node.parent == parent && node.name == name; });
    if (taken) {
        throw exceptions::CapsuleException{
            std::format("Capsule '{}': '{}' already has a child named '{}'", _path, getNodePath(parent), name)};
    }
    const auto index = static_cast<std::uint32_t>(_nodes.size());
    _nodes.push_back(CapsuleNode{.name = std::move(name), .parent = parent});
    return index;
}

std::optional<std::uint32_t> Capsule::findNode(std::string_view path) const {
    std::uint32_t current = 0;
    if (path.empty() || path == ".") {
        return current;
    }
    while (!path.empty()) {
        const std::size_t slash = path.find('/');
        const std::string_view name = path.substr(0, slash);
        const auto child = std::ranges::find_if(
            _nodes, [&](const CapsuleNode& node) { return node.parent == current && node.name == name; });
        if (child == _nodes.end()) {
            return std::nullopt;
        }
        current = static_cast<std::uint32_t>(std::distance(_nodes.begin(), child));
        path = slash == std::string_view::npos ? std::string_view{} : path.substr(slash + 1);
    }
    return current;
}

std::string Capsule::getNodePath(std::size_t index) const {
    assert(index < _nodes.size());
    if (index == 0) {
        return ".";
    }
    std::vector<std::string_view> names;
    for (std::size_t current = index; current != 0;) {
        const CapsuleNode& node = *std::next(_nodes.begin(), static_cast<std::ptrdiff_t>(current));
        names.push_back(node.name);
        current = node.parent;
    }
    std::string path;
    for (const std::string_view name : names | std::views::reverse) {
        path += path.empty() ? std::string{name} : std::format("/{}", name);
    }
    return path;
}
}  // namespace rtype::plugins::capsule
