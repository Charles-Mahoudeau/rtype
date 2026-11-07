/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Capsules
*/

#include "Capsules.hpp"

#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "core/asset/AssetFiles.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::plugins::capsule {
ecs::RunMode Capsules::getRunMode() const noexcept { return _mode; }

void Capsules::setRunMode(ecs::RunMode mode) noexcept { _mode = mode; }

std::size_t Capsules::getCount() const noexcept { return _capsules.size(); }

Capsule& Capsules::define(std::string_view path) {
    if (path.empty()) {
        throw exceptions::CapsuleException{"A capsule needs a path"};
    }
    std::string normalized = core::asset::normalizePath(path);
    const CapsuleId id = CapsuleId::fromPath(normalized);
    if (const auto found = _capsules.find(id); found != _capsules.end()) {
        Capsule& capsule = found->second;
        if (capsule.getPath() != normalized) {
            throw exceptions::CapsuleException{
                std::format("Capsule '{}' has the same id as '{}': rename one of them", normalized, capsule.getPath())};
        }
        capsule.clear();
        ++capsule._version;
        return capsule;
    }
    return _capsules.try_emplace(id, id, std::move(normalized)).first->second;
}

const Capsule* Capsules::find(CapsuleId id) const noexcept {
    const auto found = _capsules.find(id);
    return found != _capsules.end() ? &found->second : nullptr;
}

const Capsule* Capsules::find(std::string_view path) const {
    const std::string normalized = core::asset::normalizePath(path);
    const Capsule* capsule = find(CapsuleId::fromPath(normalized));
    return capsule != nullptr && capsule->getPath() == normalized ? capsule : nullptr;
}

Capsule& Capsules::edit(CapsuleId id) {
    const auto found = _capsules.find(id);
    if (found == _capsules.end()) {
        throw exceptions::CapsuleException{std::format("No capsule has the id {:#x}", id.value)};
    }
    return found->second;
}

bool Capsules::loads(ecs::Side side) const noexcept { return ecs::runsIn(side, _mode); }

bool Capsules::keeps(ecs::Presence presence) const noexcept {
    switch (presence) {
        case ecs::Presence::kServerOnly:
            return _mode != ecs::RunMode::kClient;
        case ecs::Presence::kClientOnly:
            return _mode != ecs::RunMode::kServer;
        case ecs::Presence::kBoth:
            break;
    }
    return true;
}
}  // namespace rtype::plugins::capsule
