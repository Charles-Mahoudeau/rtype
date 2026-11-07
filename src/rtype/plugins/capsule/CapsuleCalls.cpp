/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleCalls
*/

#include "CapsuleCalls.hpp"

#include <span>
#include <string>
#include <vector>

#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::capsule {
namespace {
CapsuleRoot spawn(ecs::Commands& commands, ecs::Res<Capsules> capsules, const CapsuleArguments& arguments) {
    return CapsuleRoot{.root = spawnCapsule(commands, *capsules, arguments.capsule).getEntity()};
}

CapsuleRoot load(ecs::Commands& commands, ecs::Res<Capsules> capsules, ecs::ResMut<LoadedCapsule> loaded,
                 const CapsuleArguments& arguments) {
    return CapsuleRoot{.root = loadCapsule(commands, *capsules, *loaded, arguments.capsule).getEntity()};
}

ReloadResult reload(ecs::Commands& commands, ecs::Res<Capsules> capsules,
                    ecs::Query<const CapsuleInstance, ecs::Optional<const ecs::Children>> roots,
                    const ReloadArguments& arguments) {
    for (auto [entity, instance, children] : roots) {  // no lookup by entity yet: a few roots at most
        if (entity == arguments.root) {
            const std::vector<ecs::Entity> old = children != nullptr ? children->entities : std::vector<ecs::Entity>{};
            reloadCapsule(commands, *capsules, entity, instance, old);
            return ReloadResult{.reloaded = true};
        }
    }
    return ReloadResult{.reloaded = false};
}
}  // namespace

void addCapsuleCalls(ecs::App& app) {
    app.addCall(std::string{kSpawnCall}, &spawn);
    app.addCall(std::string{kLoadCall}, &load);
    app.addCall(std::string{kReloadCall}, &reload);
}
}  // namespace rtype::plugins::capsule
