-- The player's own rider, outlined: client/looks.luau draws every rider, this tells the player which one is theirs.

local Game = require("@scripts/shared/rules")

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
