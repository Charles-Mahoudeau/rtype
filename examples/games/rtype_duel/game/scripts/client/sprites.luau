-- The look of what the server spawns: the ships and the lasers fly animated where the server puts them, and the loser
-- explodes.

local explosionScale = 1.5

-- A finished explosion: removed when its animation ends.
engine.ecs.component("game.Explosion", { fields = {}, presence = "clientOnly" })

-- The rest draws and plays: the server has none of the plugins it names.
if not engine.isClient then
	return
end

-- Update: gives the ships the server spawned their sprite, animated.
engine.ecs.system("game.DressShips", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Ship" }, without = { "rtype.SpriteRenderer" } },
	run = function(ctx)
		for entity, ship in ctx.query do
			ctx.commands:add(entity, "rtype.Transform", { position = ship.position })
			ctx.commands:add(entity, "rtype.SpriteRenderer", { layer = 1 })
			ctx.commands:add(entity, "rtype.SpriteAnimator", {
				atlas = if ship.player == 0 then Duel.assets.leftShip else Duel.assets.rightShip,
				animation = "fly",
			})
		end
	end,
})

-- Update: gives the lasers the server spawned their sprite, flying away from their shooter.
engine.ecs.system("game.DressLasers", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Laser" }, without = { "rtype.SpriteRenderer" } },
	run = function(ctx)
		for entity, laser in ctx.query do
			ctx.commands:add(entity, "rtype.Transform", { position = laser.position })
			ctx.commands:add(entity, "rtype.SpriteRenderer", { layer = 2 })
			ctx.commands:add(entity, "rtype.SpriteAnimator", {
				atlas = Duel.assets.laser,
				animation = if laser.shooter == 0 then "right" else "left",
			})
		end
	end,
})

-- Update: the ships' sprites follow the positions the server sends.
engine.ecs.system("game.FollowShips", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Ship" }, write = { "rtype.Transform" } },
	run = function(ctx)
		for _, ship, transform in ctx.query do
			transform.position = ship.position
		end
	end,
})

-- Update: so do the lasers'.
engine.ecs.system("game.FollowLasers", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Laser" }, write = { "rtype.Transform" } },
	after = { "game.FollowShips" },
	run = function(ctx)
		for _, laser, transform in ctx.query do
			transform.position = laser.position
		end
	end,
})

-- The player who just won, read by game.ExplodeLoser; nil while nobody did.
local newWinner: number? = nil
-- The ships' players by their client's id, filled by game.TrackOwners.
local playerOf: { [number]: number } = {}

-- Update: notices the Winner the server spawns, and says who won until the HUD's texts do.
engine.ecs.system("game.AnnounceWinner", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Winner" }, added = { "game.Winner" } },
	calls = { "rtype.replication.localClient" },
	run = function(ctx)
		for _, winner in ctx.query do
			newWinner = winner.player
			local me = ctx.call("rtype.replication.localClient", {})
			local mine = me.connected and playerOf[me.client] == winner.player
			print(`Player {winner.player + 1} wins! {if mine then "You win :)" else "You lose :("}`)
		end
	end,
})

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

-- Update: once someone won, the loser's ship vanishes in an explosion.
engine.ecs.system("game.ExplodeLoser", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Ship" }, write = { "rtype.SpriteRenderer" } },
	after = { "game.AnnounceWinner", "game.DressShips" },
	run = function(ctx)
		if newWinner == nil then
			return
		end
		for _, ship, sprite in ctx.query do
			if ship.player ~= newWinner then
				local color = sprite.color
				sprite.color = { x = color.x, y = color.y, z = color.z, w = 0 } -- its alpha: the ship is gone
				ctx.commands:spawn({
					["rtype.Transform"] = {
						position = ship.position,
						scale = { x = explosionScale, y = explosionScale },
					},
					["rtype.SpriteRenderer"] = { layer = 3, blend = Duel.additive },
					["rtype.SpriteAnimator"] = { atlas = Duel.assets.explosion, animation = "explode" },
					["game.Explosion"] = {},
				})
			end
		end
		newWinner = nil
	end,
})

-- Update: removes the explosions that finished.
engine.ecs.system("game.RemoveExplosions", {
	phase = "Update",
	side = "client",
	query = { with = { "game.Explosion" } },
	events = { read = { "rtype.AnimationFinished" } },
	run = function(ctx)
		local finished = {}
		for event in ctx.events.read("rtype.AnimationFinished") do
			finished[event.entity.bits] = true
		end
		for entity in ctx.query do
			if finished[entity.bits] then
				ctx.commands:despawn(entity)
			end
		end
	end,
})
