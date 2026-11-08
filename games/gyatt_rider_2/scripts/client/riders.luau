-- Which rider is the player's: client/looks.luau draws every rider with the cyclist's white image, tinted here blue for
-- the player's own and red for the others, and the player's own is outlined.

local Game = require("@scripts/shared/rules")

local ownColor = { x = 0.3, y = 0.55, z = 1 } -- blue
local enemyColor = { x = 1, y = 0.25, z = 0.25 } -- red
local ownOutline = { x = 1, y = 1, z = 1, w = 1 }
local ownOutlineThickness = 3

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
					fillColor = { x = 0, y = 0, z = 0, w = 0 },
					outlineColor = ownOutline,
					outlineThickness = ownOutlineThickness,
				})
			end
		end
	end,
})

-- Update, once the riders have their sprites (client/looks.luau): tints the player's own rider blue and the others red,
-- keeping the alpha the shield's twinkle sets.
engine.ecs.system("game.TintRiders", {
	phase = "Update",
	side = "client",
	query = { read = { "net.Owner" }, write = { "rtype.SpriteRenderer" }, with = { "game.Rider" } },
	calls = { "rtype.replication.localClient" },
	after = { "game.DressLooks" },
	before = { "game.TwinkleShields" },
	run = function(ctx)
		local me = ctx.call("rtype.replication.localClient", {})
		for _, owner, sprite in ctx.query do
			local tint = if me.connected and owner.client == me.client then ownColor else enemyColor
			local color = sprite.color
			if color.x ~= tint.x or color.y ~= tint.y or color.z ~= tint.z then
				sprite.color = { x = tint.x, y = tint.y, z = tint.z, w = color.w }
			end
		end
	end,
})
