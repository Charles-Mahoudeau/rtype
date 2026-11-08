-- The look of the riders the server spawns: the sprite of their slot, where the server puts them, and an outline
-- around the player's own rider.

local Game = require("@scripts/shared/rules")
local assets = require("@scripts/client/assets")

local riderDrawLayer = 1
local ownOutline = { x = 1, y = 1, z = 1, w = 1 }
local ownOutlineThickness = 3

-- Update: gives the riders the server spawned the sprite of their slot.
engine.ecs.system("game.DressRiders", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Rider" }, without = { "rtype.SpriteRenderer" } },
	run = function(ctx)
		local size = Game.riderSize
		for entity, rider in ctx.query do
			ctx.commands:add(entity, "rtype.Transform", { position = rider.position })
			ctx.commands:add(entity, "rtype.SpriteRenderer", {
				texture = assets.riders[rider.slot + 1],
				source = { x = 0, y = 0, z = size.x, w = size.y }, -- the whole image
				origin = { x = size.x / 2, y = size.y / 2 }, -- centred on its position, like its collider
				layer = riderDrawLayer,
			})
		end
	end,
})

-- Update: the riders' sprites follow the positions the server sends.
engine.ecs.system("game.FollowRiders", {
	phase = "Update",
	side = "client",
	query = { read = { "game.Rider" }, write = { "rtype.Transform" } },
	run = function(ctx)
		for _, rider, transform in ctx.query do
			transform.position = rider.position
		end
	end,
})

-- Update: outlines the player's own rider, once the server gave this client its id.
engine.ecs.system("game.OutlineOwnRider", {
	phase = "Update",
	side = "client",
	query = { read = { "net.Owner" }, with = { "game.Rider" }, without = { "rtype.ShapeRenderer" } },
	calls = { "rtype.replication.localClient" },
	run = function(ctx)
		local me = ctx.call("rtype.replication.localClient", {})
		if not me.connected then
			return
		end
		for entity, owner in ctx.query do
			if owner.client == me.client then
				ctx.commands:add(entity, "rtype.ShapeRenderer", {
					kind = Game.rectShape,
					size = Game.riderSize,
					cornerRadius = 12,
					fillColor = { x = 0, y = 0, z = 0, w = 0 },
					outlineColor = ownOutline,
					outlineThickness = ownOutlineThickness,
				})
			end
		end
	end,
})
