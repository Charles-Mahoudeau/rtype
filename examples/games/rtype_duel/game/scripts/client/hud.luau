-- The texts over the game: each player's name at the top of their half, and who won at the end.

-- The rest draws: the server has none of the plugins it names.
if not engine.isClient then
	return
end

local textLayer = 10 -- over every sprite
local nameSize, nameTop = 22, 12
local nameColor = { x = 1, y = 1, z = 1, w = 0.85 }
-- Shapes are drawn over every text, so the end of the game is written in the gaps of the middle column: who won between
-- the top wall and the rock, and whether it is this player between the rock and the bottom wall (server/obstacles.luau).
local winnerSize = 40
local winnerY, verdictY = 237, 363
local winColor = { x = 1, y = 0.85, z = 0.3, w = 1 }
local loseColor = { x = 1, y = 0.45, z = 0.4, w = 1 }

-- The ships' players by their client's id, filled by game.TrackOwners.
local playerOf: { [number]: number } = {}

-- Startup: each player's name, at the top of the middle of their half.
engine.ecs.system("game.SpawnNames", {
	phase = "Startup",
	side = "client",
	query = {},
	run = function(ctx)
		for player = 0, Duel.playerCount - 1 do
			ctx.commands:spawn({
				["rtype.Transform"] = { position = { x = Duel.viewWidth / 4 * (2 * player + 1), y = nameTop } },
				["rtype.TextRenderer"] = {
					text = `Player {player + 1}`,
					font = Duel.assets.font,
					size = nameSize,
					color = nameColor,
					anchor = { x = 0.5, y = 0 },
					layer = textLayer,
				},
			})
		end
	end,
})

-- Who won and who is who come from the network: run alone (the editor), there are no players.
if not Duel.networked then
	return
end

-- Update: the ships' players by client, for game.AnnounceWinner.
engine.ecs.system("game.TrackOwners", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Ship", "net.Owner" } },
	before = { "game.AnnounceWinner" },
	run = function(ctx)
		table.clear(playerOf)
		for _, ship, owner in ctx.query do
			playerOf[owner.client] = ship.player
		end
	end,
})

-- Update: once the server spawns the Winner, says in the middle of the arena who won, and whether it is this player.
engine.ecs.system("game.AnnounceWinner", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Winner" }, added = { "game.Winner" } },
	calls = { "rtype.replication.localClient" },
	run = function(ctx)
		for _, winner in ctx.query do
			local me = ctx.call("rtype.replication.localClient", {})
			local mine = me.connected and playerOf[me.client] == winner.player
			local lines = {
				{ text = `Player {winner.player + 1} wins!`, y = winnerY },
				{ text = if mine then "You win :)" else "You lose :(", y = verdictY },
			}
			for _, line in lines do
				ctx.commands:spawn({
					["rtype.Transform"] = { position = { x = Duel.viewWidth / 2, y = line.y } },
					["rtype.TextRenderer"] = {
						text = line.text,
						font = Duel.assets.font,
						size = winnerSize,
						color = if mine then winColor else loseColor,
						anchor = { x = 0.5, y = 0.5 },
						layer = textLayer,
					},
				})
			end
		end
	end,
})
