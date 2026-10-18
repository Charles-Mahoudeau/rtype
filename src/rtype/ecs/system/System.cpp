/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** System
*/

#include "System.hpp"

#include <string_view>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
std::string_view System::getName() const noexcept { return _name; }

Side System::getSide() const noexcept { return _side; }

const SystemAccess& System::getAccess() const noexcept { return _access; }

Tick System::getLastRun() const noexcept { return _lastRun; }

void System::setSide(Side side) noexcept { _side = side; }

void System::run() {
    const Tick thisRun = _world->getChangeTick();
    _body->run(SystemContext{.world = _world, .commands = &_commands, .lastRun = _lastRun, .thisRun = thisRun});
    _lastRun = thisRun;
    _world->advanceChangeTick();  // the next run starts after this tick: what this run wrote is not reported to it
}

void System::flushCommands() { _commands.flush(); }
}  // namespace rtype::ecs
