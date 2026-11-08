-- The shots: a rider fires up, down, left or right with the arrows, every Game.fireCooldown while one is held. A shot
-- is instant: a ray goes from the rider until the first rider, vehicle or cyclist it meets, or the view's edge, and
-- leaves a beam there for Game.beamLifetime. What it meets gets a game.Hit, which server/hits.luau resolves. With the
-- sniper boost, a shot goes through everything to the view's edge, and hits every rider and cyclist on its way:
-- game.PierceShots finds them by their boxes, since a ray stops at the first body and ignores only one.

local Game = require("@scripts/shared/rules")
local Match = require("@scripts/server/match")
local Boosts = require("@scripts/server/boosts")

-- What a shot meets: the riders, which are sensors, and the traffic.
local shotMask = bit32.bor(Game.riderLayer, Game.vehicleLayer, Game.cyclistLayer)

-- This tick's sniper shots, from game.Fire to game.PierceShots: where each goes, and its shooter.
local piercing: { { from: { x: number, y: number }, to: { x: number, y: number }, slot: number, shooter: number } } = {}

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

--- Fires a shot of the rider in @p slot from @p from: spawns a game.Hit for the first thing the ray meets, and returns
--- how far the shot goes.
local function shoot(ctx, shooter, slot: number, from, direction, reach: number): number
	local hit = ctx.call("rtype.physics.raycast", {
		origin = from,
		direction = direction,
		maxDistance = reach,
		mask = shotMask,
		ignore = shooter, -- not its own rider
		includeSensors = true, -- the riders are sensors
	})
	if not hit.hit then
		return reach
	end
	ctx.commands:spawn({ ["game.Hit"] = { victim = hit.entity, shooter = slot } })
	return hit.distance
end

--- Whether the axis-aligned segment from @p from to @p to crosses the box of @p size centred on @p at.
local function crosses(from, to, at, size): boolean
	local halfX, halfY = size.x / 2, size.y / 2
	return math.max(from.x, to.x) >= at.x - halfX
		and math.min(from.x, to.x) <= at.x + halfX
		and math.max(from.y, to.y) >= at.y - halfY
		and math.min(from.y, to.y) <= at.y + halfY
end

-- FixedUpdate, once the physics stepped, so that the ray meets the scene of this tick: a rider whose gun is ready
-- fires the way its arrows point, and pays Game.shotCost while the match races. The score is read, and patched only
-- when a shot costs: a system that goes through a component in `write` marks it changed, which sends it every tick.
engine.ecs.system("game.Fire", {
	phase = "FixedUpdate",
	side = "server",
	query = {
		read = { "game.Rider", "game.Position", "game.Boosts", "game.Score" },
		write = { "game.Gun" },
		with = { "net.PlayerInput" },
	},
	calls = { "rtype.replication.readInput", "rtype.physics.raycast" },
	after = { "net.applyInputs", "rtype.Physics.Events" },
	run = function(ctx)
		for entity, rider, position, boosts, score, gun in ctx.query do
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
			if Match.racing() and score.points > 0 then
				ctx.commands:patch(entity, "game.Score", { points = math.max(0, score.points - Game.shotCost) })
			end
			local from = position.value
			local reach = reachToEdge(from, direction)
			local length = reach
			if Boosts.sniping(boosts) then
				local to = { x = from.x + direction.x * reach, y = from.y + direction.y * reach }
				table.insert(piercing, { from = from, to = to, slot = rider.slot, shooter = entity.bits })
			else
				length = shoot(ctx, entity, rider.slot, from, direction, reach)
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

-- FixedUpdate, after the shots of the tick: each sniper shot hits every rider and cyclist whose box it crosses, but
-- its shooter. The vehicles do not stop it.
engine.ecs.system("game.PierceShots", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Position", "rtype.Collider" }, optionalRead = { "game.Rider", "game.Cyclist" } },
	after = { "game.Fire" },
	before = { "game.CollectHits" },
	run = function(ctx)
		if #piercing == 0 then
			return
		end
		for entity, position, collider, rider, cyclist in ctx.query do
			if rider == nil and cyclist == nil then
				continue -- a vehicle
			end
			for _, shot in piercing do
				if entity.bits ~= shot.shooter and crosses(shot.from, shot.to, position.value, collider.size) then
					ctx.commands:spawn({ ["game.Hit"] = { victim = entity, shooter = shot.slot } })
				end
			end
		end
		table.clear(piercing)
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
