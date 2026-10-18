/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RunModeTests
*/

#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace {
using rtype::ecs::RunMode;
using rtype::ecs::runsIn;
using rtype::ecs::Side;

// A dedicated server runs server systems and systems of both sides.
static_assert(runsIn(Side::kServer, RunMode::kServer));
static_assert(runsIn(Side::kBoth, RunMode::kServer));
static_assert(!runsIn(Side::kClient, RunMode::kServer));

// A client runs client systems and systems of both sides.
static_assert(runsIn(Side::kClient, RunMode::kClient));
static_assert(runsIn(Side::kBoth, RunMode::kClient));
static_assert(!runsIn(Side::kServer, RunMode::kClient));

// Single player runs everything in one world.
static_assert(runsIn(Side::kServer, RunMode::kStandalone));
static_assert(runsIn(Side::kClient, RunMode::kStandalone));
static_assert(runsIn(Side::kBoth, RunMode::kStandalone));
}  // namespace
