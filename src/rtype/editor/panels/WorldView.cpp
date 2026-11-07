/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldView
*/

#include "WorldView.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace {
constexpr std::size_t kNamesInLine = 3;  ///< How many component names describeEntity() lists.

/// @return @p text in lower case.
std::string lowered(std::string_view text) {
    std::string result{text};
    std::ranges::transform(result, result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}
}  // namespace

std::string shortName(std::string_view name) {
    const std::size_t dot = name.rfind('.');
    return std::string{dot == std::string_view::npos ? name : name.substr(dot + 1)};
}

std::vector<ecs::Entity> collectEntities(const ecs::World& world, std::string_view filter) {
    const ecs::Components& components = world.getComponents();
    const std::string wanted = lowered(filter);
    std::vector<ecs::Entity> entities;
    const std::size_t count = components.getRegistry().getCount();
    for (std::size_t index = 0; index < count; ++index) {
        const auto id = static_cast<ecs::ComponentId>(index);
        const ecs::ComponentInfo* info = components.getRegistry().findById(id);
        const auto* pool = components.findPool(id);
        if (info == nullptr || pool == nullptr || (!wanted.empty() && !lowered(info->type.name).contains(wanted))) {
            continue;
        }
        const auto members = pool->getEntities();
        entities.insert(entities.end(), members.begin(), members.end());
    }
    std::ranges::sort(entities, {}, &ecs::Entity::getIndex);
    const auto duplicates = std::ranges::unique(entities);
    entities.erase(duplicates.begin(), duplicates.end());
    return entities;
}

std::string describeEntity(const ecs::World& world, ecs::Entity entity) {
    const ecs::Components& components = world.getComponents();
    std::string names;
    std::size_t listed = 0;
    std::size_t total = 0;
    const std::size_t count = components.getRegistry().getCount();
    for (std::size_t index = 0; index < count; ++index) {
        const auto id = static_cast<ecs::ComponentId>(index);
        const ecs::ComponentInfo* info = components.getRegistry().findById(id);
        if (info == nullptr || !components.has(entity, id)) {
            continue;
        }
        ++total;
        if (listed < kNamesInLine) {
            names += (listed == 0 ? "" : ", ") + shortName(info->type.name);
            ++listed;
        }
    }
    return std::format("#{}  {}{}", entity.getIndex(), names, total > listed ? ", ..." : "");
}

}  // namespace rtype::editor::panels
