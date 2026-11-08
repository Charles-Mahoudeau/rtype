-- The riders: they move in every direction with ZQSD, inside the roads and the view, faster with the speed boost.
-- Vehicles do not block them: a vehicle that touches a rider hits it.

local Game = require("@scripts/shared/rules")
local Boosts = require("@scripts/server/boosts")

-- FixedUpdate, after the inputs of the tick are applied and before the Transforms drive the bodies.
engine.ecs.system("game.MoveRiders", {
	phase = "FixedUpdate",
	side = "server",
	query = {
		read = { "game.Boosts" },
		write = { "game.Position", "rtype.Transform" },
		with = { "game.Rider", "net.PlayerInput" },
	},
	calls = { "rtype.replication.readInput" },
	after = { "net.applyInputs", "rtype.Physics.SyncBodies" },
	before = { "rtype.Physics.SyncToPhysics" },
	run = function(ctx)
		local halfX, halfY = Game.riderSize.x / 2, Game.riderSize.y / 2
		for entity, boosts, current, transform in ctx.query do
			local move = ctx.call("rtype.replication.readInput", { entity = entity, action = "move" }).vector
			-- A diagonal is as fast as a straight line.
			local length = math.sqrt(move.x * move.x + move.y * move.y)
			if length > 1 then
				move = { x = move.x / length, y = move.y / length }
			end
			-- Up is +1 in the input, and Y goes down on screen.
			local step = Game.riderSpeed * Boosts.speedFactor(boosts) * ctx.time.fixedDelta
			local position = {
				x = math.clamp(current.value.x + move.x * step, Game.roadsLeft + halfX, Game.roadsRight - halfX),
				y = math.clamp(current.value.y - move.y * step, halfY, Game.viewHeight - halfY),
			}
			-- A write marks the position changed, which sends it to the clients: only a rider that moved is written.
			if position.x == current.value.x and position.y == current.value.y then
				continue
			end
			transform.position = position
			current.value = position
		end
	end,
})
