/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsulePlugin
*/

#include "CapsulePlugin.hpp"

#include <string_view>

#include "plugins/capsule/Capsules.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/resource/Resources.hpp"

namespace rtype::plugins::capsule {
std::string_view CapsulePlugin::getName() const noexcept { return kName; }

void CapsulePlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<Capsules>();
    resources.requireMut<Capsules>()->setRunMode(app.getSchedules().getRunMode());  // a spawn keeps this side's
}
}  // namespace rtype::plugins::capsule
