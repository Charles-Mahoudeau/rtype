/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Hud
*/

#pragma once

#include "Components.hpp"
#include "Resources.hpp"
#include "plugins/text/FontAssets.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/replication/client/ClientState.hpp"
#include "rtype/replication/component/Owner.hpp"

// Client systems: the texts of the game (MSDF).

namespace rtype_duel {

/// @brief Startup: reads the font, and writes each player's name at the top of their half of the arena.
void spawnNames(rtype::ecs::ResMut<rtype::plugins::text::FontAssets> fonts, rtype::ecs::ResMut<GameFonts> game,
                rtype::ecs::Commands& commands);

/// @brief Update: tells the player who won, once, in the middle of the arena.
void announceWinner(rtype::ecs::Commands& commands, rtype::ecs::Res<rtype::replication::client::ClientState> client,
                    rtype::ecs::Res<GameFonts> game, rtype::ecs::Query<const Winner, rtype::ecs::Added<Winner>> winners,
                    rtype::ecs::Query<const Ship, const rtype::replication::component::Owner> ships);

}  // namespace rtype_duel
