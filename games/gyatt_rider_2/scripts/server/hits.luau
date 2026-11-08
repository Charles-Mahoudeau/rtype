-- The hits: a vehicle that runs over a rider and a shot that reaches anything spawn a game.Hit, and the hits are
-- resolved here, in one place, whatever made them. A rider is not removed by a hit: it loses Game.hitLoss of its points
-- and keeps riding, and its shooter wins Game.hitReward plus what it lost; then its shield keeps it from being hit
-- again for Game.hitShield seconds. A cyclist is: knocked off the road, it gives Game.hitReward (and its boost) once. A
-- vehicle just stops the shot. The points count only while the match races.

local Game = require("@scripts/shared/rules")
local Match = require("@scripts/server/match")
local Shield = require("@scripts/server/shield")

-- This tick's hits by victim (entity bits): the shooter of each, filled by game.CollectHits for the systems that
-- resolve them.
local hitsOf: { [number]: { number } } = {}

-- This tick's points won by shooters, by their slot: filled while the hits are resolved, paid by game.PayShooters, so
-- that a victim loses its share before anyone is paid.
local winningsOf: { [number]: number } = {}

--- Gives @p points to the shooter in @p slot, paid at the end of the tick.
local function win(slot: number, points: number)
	winningsOf[slot] = (winningsOf[slot] or 0) + points
end

-- FixedUpdate, once the physics sent this tick's events: a vehicle that enters a rider (a sensor) hits it. The event
-- names the sensor and what entered it; their colliders' layers tell a rider and a vehicle.
engine.ecs.system("game.HitByTraffic", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "rtype.Collider" } },
	events = { read = { "rtype.TriggerEntered" } },
	after = { "rtype.Physics.Events" },
	run = function(ctx)
		local entries = {}
		for entered in ctx.events.read("rtype.TriggerEntered") do
			table.insert(entries, { rider = entered.trigger, other = entered.other })
		end
		if #entries == 0 then
			return
		end
		local layerOf = {}
		for entity, collider in ctx.query do
			layerOf[entity.bits] = collider.layer
		end
		for _, entry in entries do
			if layerOf[entry.rider.bits] == Game.riderLayer and layerOf[entry.other.bits] == Game.vehicleLayer then
				ctx.commands:spawn({ ["game.Hit"] = { victim = entry.rider, shooter = -1 } })
			end
		end
	end,
})

-- FixedUpdate, once this tick's hits are spawned: groups them by victim for the systems that resolve them, and
-- despawns them.
engine.ecs.system("game.CollectHits", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Hit" } },
	after = { "game.HitByTraffic", "game.Fire" },
	run = function(ctx)
		table.clear(hitsOf)
		for entity, hit in ctx.query do
			local shooters = hitsOf[hit.victim.bits] or {}
			table.insert(shooters, hit.shooter)
			hitsOf[hit.victim.bits] = shooters
			ctx.commands:despawn(entity)
		end
	end,
})

-- FixedUpdate: applies each rider's hits of this tick, unless its shield is on: each one takes Game.hitLoss of its
-- points, which its shooter wins with Game.hitReward, then the rider is shielded.
engine.ecs.system("game.ResolveRiderHits", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Rider" }, write = { "game.Score", "game.Shield", "game.ShieldClock" } },
	after = { "game.CollectHits" },
	run = function(ctx)
		if next(hitsOf) == nil then
			return
		end
		local counts = Match.racing()
		for entity, rider, score, shield, clock in ctx.query do
			local hits = hitsOf[entity.bits]
			if hits == nil or shield.active then
				continue
			end
			for _, shooter in hits do
				local by = if shooter < 0 then "the traffic" else `player {shooter + 1}`
				print(`[server] player {rider.slot + 1} hit by {by}`)
				if not counts then
					continue
				end
				local lost = math.floor(score.points * Game.hitLoss)
				if lost > 0 then
					score.points -= lost
				end
				if shooter >= 0 then
					win(shooter, Game.hitReward + lost)
				end
			end
			Shield.grant(shield, clock, Shield.afterHit)
		end
	end,
})

-- FixedUpdate: a cyclist shot this tick is knocked off the road, by the first shot that reached it.
engine.ecs.system("game.ResolveCyclistHits", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Cyclist" } },
	after = { "game.CollectHits" },
	run = function(ctx)
		if next(hitsOf) == nil then
			return
		end
		for entity in ctx.query do
			local shooters = hitsOf[entity.bits]
			if shooters then
				print(`[server] player {shooters[1] + 1} shot a cyclist`)
				if Match.racing() then
					win(shooters[1], Game.hitReward)
				end
				ctx.commands:despawn(entity)
			end
		end
	end,
})

-- FixedUpdate, once every hit of the tick is resolved: pays the shooters what they won.
engine.ecs.system("game.PayShooters", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Rider" }, write = { "game.Score" } },
	after = { "game.ResolveRiderHits", "game.ResolveCyclistHits" },
	run = function(ctx)
		if next(winningsOf) == nil then
			return
		end
		for _, rider, score in ctx.query do
			local points = winningsOf[rider.slot]
			if points then
				score.points += points
			end
		end
		table.clear(winningsOf)
	end,
})
