-- The ships: they move in their half of the arena, sliding along the obstacles, and fire on a click until someone has
-- won. One shot in Duel.misfireOdds misfires: a Misfire replaces the laser, for the clients to play its sound.

-- The server's systems only, and only with the network: the ships are the players'.
if not engine.isServer or not Duel.networked then
	return
end

-- FixedUpdate, after the inputs of the tick are applied and before the Transforms drive the bodies.
engine.ecs.system("game.MoveShips", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "game.Ship", "rtype.Transform" }, with = { "rtype.Collider", "net.PlayerInput" } },
	calls = { "rtype.replication.readInput", "rtype.physics.moveAndSlide", "rtype.capsule.spawn" },
	after = { "net.applyInputs", "rtype.Physics.SyncBodies" },
	before = { "rtype.Physics.SyncToPhysics" },
	run = function(ctx)
		if Duel.over then
			return
		end
		for entity, ship, transform in ctx.query do
			local move = ctx.call("rtype.replication.readInput", { entity = entity, action = "move" }).vector
			-- Up is +1 in the input, and Y goes down on screen.
			local step = Duel.shipSpeed * ctx.time.fixedDelta
			local slid = ctx.call("rtype.physics.moveAndSlide", {
				entity = entity,
				from = transform.position,
				delta = { x = move.x * step, y = -move.y * step },
			})
			local left = ship.player == 0
			local minX = if left then Duel.shipHalfSize.x else Duel.middleX + Duel.shipHalfSize.x
			local maxX = if left then Duel.middleX - Duel.shipHalfSize.x else Duel.viewWidth - Duel.shipHalfSize.x
			local position = {
				x = math.clamp(slid.x, minX, maxX),
				y = math.clamp(slid.y, Duel.shipHalfSize.y, Duel.viewHeight - Duel.shipHalfSize.y),
			}
			transform.position = position
			ship.position = position
			if ctx.call("rtype.replication.readInput", { entity = entity, action = "shoot" }).pressed then
				Duel.fire(ctx, ship.player, position)
			end
		end
	end,
})

--- Fires a laser from a player's ship at @p position, or misfires.
function Duel.fire(ctx, player: number, position)
	local direction = Duel.directionOf(player)
	local muzzle = {
		x = position.x + direction * (Duel.shipHalfSize.x + Duel.laserHalfSize.x + 1),
		y = position.y,
	}
	if math.random(1, Duel.misfireOdds) == 1 then
		ctx.commands:spawn({
			["game.Misfire"] = { position = muzzle, shooter = player },
			["game.MisfireTimer"] = { remaining = Duel.misfireLifetime },
		})
		return
	end
	-- The laser's actor (assets/actors/laser.actor) is the body; the shot gives where, who, which way and whom it hits.
	engine.capsule.spawn(ctx, Duel.laser, {
		["game.Laser"] = { position = muzzle, shooter = player },
		["rtype.Transform"] = { position = muzzle },
		["rtype.Collider"] = { mask = bit32.bor(Duel.obstacleLayer, Duel.shipLayerOf(1 - player)) },
		["rtype.Velocity"] = { linear = { x = direction * Duel.laserSpeed, y = 0 } },
	})
end

-- FixedUpdate: removes the Misfires once a snapshot had time to carry them to the clients.
engine.ecs.system("game.ExpireMisfires", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "game.MisfireTimer" } },
	run = function(ctx)
		for entity, timer in ctx.query do
			timer.remaining -= ctx.time.fixedDelta
			if timer.remaining <= 0 then
				ctx.commands:despawn(entity)
			end
		end
	end,
})
