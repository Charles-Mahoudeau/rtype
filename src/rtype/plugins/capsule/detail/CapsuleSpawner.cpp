/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleSpawner
*/

#include "CapsuleSpawner.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <iterator>
#include <string>
#include <vector>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"

namespace rtype::plugins::capsule::detail {
namespace {
// NOLINTNEXTLINE(misc-no-recursion): the stack stops a capsule that contains itself.
void checkNodes(const Capsules& capsules, const Capsule& capsule, std::vector<const Capsule*>& stack) {
    stack.push_back(&capsule);
    const auto nodes = capsule.getNodes();
    std::vector<bool> kept(nodes.size(), false);
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const CapsuleNode& node = *std::next(nodes.begin(), static_cast<std::ptrdiff_t>(index));
        if (index != 0 && (!kept.at(node.parent) || !capsules.loads(node.side))) {
            continue;  // skipped with its subtree, as the spawn does
        }
        kept.at(index) = true;
        if (!node.instance) {
            continue;
        }
        const Capsule* inner = capsules.find(node.instance->id);
        if (inner == nullptr) {
            throw exceptions::CapsuleException{std::format("Capsule '{}' spawns '{}' at '{}', which no capsule has",
                                                           capsule.getPath(), node.instance->path,
                                                           capsule.getNodePath(index))};
        }
        if (std::ranges::find(stack, inner) != stack.end()) {
            std::string chain;
            for (const Capsule* outer : stack) {
                chain += std::format("'{}' -> ", outer->getPath());
            }
            throw exceptions::CapsuleException{
                std::format("Capsule '{}' contains itself: {}'{}'", inner->getPath(), chain, inner->getPath())};
        }
        checkNodes(capsules, *inner, stack);
    }
    stack.pop_back();
}
}  // namespace

void checkCapsule(const Capsules& capsules, const Capsule& capsule) {
    std::vector<const Capsule*> stack;
    checkNodes(capsules, capsule, stack);
}
}  // namespace rtype::plugins::capsule::detail
