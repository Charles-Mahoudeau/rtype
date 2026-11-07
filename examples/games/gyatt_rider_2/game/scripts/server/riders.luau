-- The riders: they move in every direction with ZQSD, inside the roads and the view. Vehicles do not block them: a
-- vehicle that touches a rider hits it.

-- The server's systems only: clients have none of the systems they are ordered against.
if not engine.isServer then
	return
end

-- FixedUpdate, after the inputs of the tick are applied and before the Transforms drive the bodies.
engine.ecs.system("game.MoveRiders", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "game.Rider", "rtype.Transform" }, with = { "net.PlayerInput" } },
	calls = { "rtype.replication.readInput" },
	after = { "net.applyInputs", "rtype.Physics.SyncBodies" },
	before = { "rtype.Physics.SyncToPhysics" },
	run = function(ctx)
		local half = Game.sizeOf(Game.riderTiles)
		half.x /= 2
		half.y /= 2
		for entity, rider, transform in ctx.query do
			local move = ctx.call("rtype.replication.readInput", { entity = entity, action = "move" }).vector
			-- A diagonal is as fast as a straight line.
			local length = math.sqrt(move.x * move.x + move.y * move.y)
			if length > 1 then
				move = { x = move.x / length, y = move.y / length }
			end
			-- Up is +1 in the input, and Y goes down on screen.
			local step = Game.riderSpeed * ctx.time.fixedDelta
			local position = {
				x = math.clamp(rider.position.x + move.x * step, Game.roadsLeft + half.x, Game.roadsRight - half.x),
				y = math.clamp(rider.position.y - move.y * step, half.y, Game.viewHeight - half.y),
			}
			transform.position = position
			rider.position = position
		end
	end,
})
