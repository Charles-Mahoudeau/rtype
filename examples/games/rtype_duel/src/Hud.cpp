/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Hud
*/

#include "Hud.hpp"

#include <cstdint>
#include <format>
#include <optional>

#include "Constants.hpp"
#include "plugins/text/TextLayout.hpp"
#include "plugins/text/TextRenderer.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/network/Types.hpp"

using namespace rtype;

namespace rtype_duel {

void spawnNames(ecs::ResMut<plugins::text::FontAssets> fonts, ecs::ResMut<GameFonts> game, ecs::Commands& commands) {
    game->text = fonts->load(kFont);
    for (std::uint32_t player = 0; player < kPlayerCount; ++player) {
        const float x = (kViewWidth / 4.0F) * static_cast<float>((2 * player) + 1);  // The middle of their half.
        commands.spawn()
            .add(plugins::transform::Transform{.position = {x, kNameTop}})
            .add(plugins::text::TextRenderer{.text = std::format("Player {}", player + 1),
                                             .font = game.get().text,
                                             .size = kNameSize,
                                             .color = kNameColor,
                                             .anchor = {0.5F, 0.0F},
                                             .layer = kTextLayer});
    }
}

void announceWinner(ecs::Commands& commands, ecs::Res<replication::client::ClientState> client,
                    ecs::Res<GameFonts> game, ecs::Query<const Winner, ecs::Added<Winner>> winners,
                    ecs::Query<const Ship, const replication::component::Owner> ships) {
    const std::optional<network::ClientId> me = client->clientId();
    for (auto [entity, winner] : winners) {
        bool mine = false;
        for (auto [shipEntity, ship, owner] : ships) {
            mine = mine || (me && ship.player == winner.player && owner.client == *me);
        }
        commands.spawn()
            .add(plugins::transform::Transform{.position = {kViewWidth / 2.0F, kViewHeight / 2.0F}})
            .add(plugins::text::TextRenderer{
                .text = std::format("Player {} wins!\n{}", winner.player + 1, mine ? "You win :)" : "You lose :("),
                .font = game->text,
                .size = kWinnerSize,
                .color = mine ? kWinColor : kLoseColor,
                .align = plugins::text::TextAlign::kCenter,
                .anchor = {0.5F, 0.5F},
                .layer = kTextLayer});
    }
}

}  // namespace rtype_duel
