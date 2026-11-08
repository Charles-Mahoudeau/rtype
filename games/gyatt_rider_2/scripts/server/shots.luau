-- The shots: a rider fires up, down, left or right with the arrows, every Game.fireCooldown while one is held. A shot
-- is instant: a ray goes from the rider until the first rider, vehicle or cyclist it meets, or the view's edge, and
-- leaves a beam there for Game.beamLifetime. What it meets gets a game.Hit, which server/hits.luau resolves.

local Game = require("@scripts/shared/rules")

-- What a shot meets: the riders, which are sensors, and the traffic.
local shotMask = bit32.bor(Game.riderLayer, Game.vehicleLayer, Game.cyclistLayer)

--- The direction of a shot from the fire action's vector: its strongest axis, as a unit vector in the view (Y down),
--- or nil when no arrow is held.
local function shotDirection(fire: { x: number, y: number }): { x: number, y: number }?
	if fire.x == 0 and fire.y == 0 then
		return nil
	end
	if math.abs(fire.x) > math.abs(fire.y) then
		return { x = math.sign(fire.x), y = 0 }
	end
	-- Up is +1 in the input, and Y goes down on screen.
	return { x = 0, y = -math.sign(fire.y) }
end

--- How far a shot from @p from goes in @p direction before it leaves the roads.
local function reachToEdge(from: { x: number, y: number }, direction: { x: number, y: number }): number
	if direction.x > 0 then
		return Game.roadsRight - from.x
	elseif direction.x < 0 then
		return from.x - Game.roadsLeft
	elseif direction.y > 0 then
		return Game.viewHeight - from.y
	end
	return from.y
end

-- FixedUpdate, once the physics stepped, so that the ray meets the scene of this tick: a rider whose gun is ready
-- fires the way its arrows point.
engine.ecs.system("game.Fire", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Rider", "game.Position" }, write = { "game.Gun" }, with = { "net.PlayerInput" } },
	calls = { "rtype.replication.readInput", "rtype.physics.raycast" },
	after = { "net.applyInputs", "rtype.Physics.Events" },
	run = function(ctx)
		for entity, rider, position, gun in ctx.query do
			if gun.cooldown > 0 then
				gun.cooldown = math.max(0, gun.cooldown - ctx.time.fixedDelta)
				continue
			end
			local fire = ctx.call("rtype.replication.readInput", { entity = entity, action = "fire" }).vector
			local direction = shotDirection(fire)
			if direction == nil then
				continue
			end
			gun.cooldown = Game.fireCooldown
			local from = position.value
			local reach = reachToEdge(from, direction)
			local hit = ctx.call("rtype.physics.raycast", {
				origin = from,
				direction = direction,
				maxDistance = reach,
				mask = shotMask,
				ignore = entity, -- not its own rider
				includeSensors = true, -- the riders are sensors
			})
			local length = if hit.hit then hit.distance else reach
			if hit.hit then
				ctx.commands:spawn({ ["game.Hit"] = { victim = hit.entity, shooter = rider.slot } })
			end
			ctx.commands:spawn({
				["game.Beam"] = {
					from = from,
					to = { x = from.x + direction.x * length, y = from.y + direction.y * length },
					shooter = rider.slot,
				},
				["game.BeamTimer"] = { remaining = Game.beamLifetime },
			})
		end
	end,
})

-- FixedUpdate: removes the beams once a snapshot had time to carry them to the clients.
engine.ecs.system("game.ExpireBeams", {
	phase = "FixedUpdate",
	side = "server",
	query = { write = { "game.BeamTimer" } },
	run = function(ctx)
		for entity, timer in ctx.query do
			timer.remaining -= ctx.time.fixedDelta
			if timer.remaining <= 0 then
				ctx.commands:despawn(entity)
			end
		end
	end,
})
