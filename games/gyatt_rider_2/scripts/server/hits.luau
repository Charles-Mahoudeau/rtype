-- The hits: a vehicle that runs over a rider and a shot that reaches anything spawn a game.Hit, and the hits are
-- resolved here, in one place, whatever made them. A rider is not removed by a hit: it loses points and keeps riding. A
-- cyclist is: knocked off the road, it gives its boost once. A vehicle just stops the shot.

local Game = require("@scripts/shared/rules")

-- This tick's hits by victim (entity bits): the shooter of each, filled by game.CollectHits for the systems that
-- resolve them.
local hitsOf: { [number]: { number } } = {}

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

-- FixedUpdate: applies each rider's hits of this tick.
engine.ecs.system("game.ResolveRiderHits", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Rider" } },
	after = { "game.CollectHits" },
	run = function(ctx)
		if next(hitsOf) == nil then
			return
		end
		for entity, rider in ctx.query do
			for _, shooter in hitsOf[entity.bits] or {} do
				local by = if shooter < 0 then "the traffic" else `player {shooter + 1}`
				print(`[server] player {rider.slot + 1} hit by {by}`)
			end
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
				ctx.commands:despawn(entity)
			end
		end
	end,
})
