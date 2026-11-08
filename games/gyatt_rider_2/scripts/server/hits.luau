-- The hits: a vehicle that runs over a rider and a shot that reaches anything spawn a game.Hit, and the hits are
-- resolved here, in one place, whatever made them. A rider is not removed by a hit: it loses Game.hitLoss of its points
-- and keeps riding, and its shooter wins Game.hitReward plus what it lost; then its shield keeps it from being hit
-- again for Game.hitShield seconds. A cyclist is: knocked off the road, it gives Game.hitReward and its boost once. A
-- vehicle just stops the shot. The points count only while the match races.

local Game = require("@scripts/shared/rules")
local Match = require("@scripts/server/match")
local Shield = require("@scripts/server/shield")
local Boosts = require("@scripts/server/boosts")

-- This tick's hits by victim (entity bits): the shooter of each, filled by game.CollectHits for the systems that
-- resolve them.
local hitsOf: { [number]: { number } } = {}

-- This tick's points and boosts won by shooters, by their slot: filled while the hits are resolved, given by
-- game.RewardShooters, so that a victim loses its share before anyone is paid.
local winningsOf: { [number]: number } = {}
local boostsOf: { [number]: { number } } = {}

--- Gives @p points to the shooter in @p slot, paid at the end of the tick.
local function win(slot: number, points: number)
	winningsOf[slot] = (winningsOf[slot] or 0) + points
end

--- Gives @p boost to the shooter in @p slot, at the end of the tick.
local function winBoost(slot: number, boost: number)
	local boosts = boostsOf[slot] or {}
	table.insert(boosts, boost)
	boostsOf[slot] = boosts
end

-- What is inside each rider: by the rider's entity bits, the traffic in it, kept by game.HitByTraffic from the
-- physics' TriggerEntered and TriggerExited.
local insideOf: { [number]: { [number]: boolean } } = {}

-- FixedUpdate, once the physics sent this tick's events: a rider that has a vehicle or a cyclist inside it (it is a
-- sensor) is hit, again and again while it stays inside, as soon as its shield is down. The events name the sensor and
-- what entered or left it; their colliders' layers tell a rider and the traffic. A vehicle that is removed sends no
-- TriggerExited, so what no longer exists is forgotten each tick.
engine.ecs.system("game.HitByTraffic", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "rtype.Collider" }, optionalRead = { "game.Shield" } },
	events = { read = { "rtype.TriggerEntered", "rtype.TriggerExited" } },
	after = { "rtype.Physics.Events" },
	run = function(ctx)
		local layerOf, shieldedOf, riderOf = {}, {}, {}
		for entity, collider, shield in ctx.query do
			layerOf[entity.bits] = collider.layer
			if collider.layer == Game.riderLayer then
				riderOf[entity.bits] = entity
				shieldedOf[entity.bits] = shield ~= nil and shield.active
			end
		end
		for entered in ctx.events.read("rtype.TriggerEntered") do
			local other = layerOf[entered.other.bits]
			if riderOf[entered.trigger.bits] and (other == Game.vehicleLayer or other == Game.cyclistLayer) then
				local inside = insideOf[entered.trigger.bits] or {}
				inside[entered.other.bits] = true
				insideOf[entered.trigger.bits] = inside
			end
		end
		for exited in ctx.events.read("rtype.TriggerExited") do
			local inside = insideOf[exited.trigger.bits]
			if inside then
				inside[exited.other.bits] = nil
			end
		end
		for riderBits, inside in insideOf do
			local rider = riderOf[riderBits]
			if rider == nil then
				insideOf[riderBits] = nil -- the rider left the game
				continue
			end
			for otherBits in inside do
				if layerOf[otherBits] == nil then
					inside[otherBits] = nil -- the vehicle left the road
				end
			end
			if next(inside) ~= nil and not shieldedOf[riderBits] then
				ctx.commands:spawn({ ["game.Hit"] = { victim = rider, shooter = -1 } })
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
			Shield.grant(shield, clock, Shield.afterHit, true)
		end
	end,
})

-- FixedUpdate: a cyclist shot this tick is knocked off the road, and gives its points and its boost to the first shot
-- that reached it.
engine.ecs.system("game.ResolveCyclistHits", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Cyclist" } },
	after = { "game.CollectHits" },
	run = function(ctx)
		if next(hitsOf) == nil then
			return
		end
		for entity, cyclist in ctx.query do
			local shooters = hitsOf[entity.bits]
			if shooters then
				print(`[server] player {shooters[1] + 1} shot a cyclist carrying boost {cyclist.boost}`)
				if Match.racing() then
					win(shooters[1], Game.hitReward)
				end
				if cyclist.boost ~= Game.noBoost then
					winBoost(shooters[1], cyclist.boost)
				end
				ctx.commands:despawn(entity)
			end
		end
	end,
})

-- FixedUpdate, once every hit of the tick is resolved: gives the shooters the points and the boosts they won.
engine.ecs.system("game.RewardShooters", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Rider" }, write = { "game.Score", "game.Shield", "game.ShieldClock", "game.Boosts" } },
	after = { "game.ResolveRiderHits", "game.ResolveCyclistHits" },
	run = function(ctx)
		if next(winningsOf) == nil and next(boostsOf) == nil then
			return
		end
		local counts = Match.racing()
		for _, rider, score, shield, clock, boosts in ctx.query do
			local points = winningsOf[rider.slot]
			if points then
				score.points += points
			end
			for _, boost in boostsOf[rider.slot] or {} do
				Boosts.give(boost, counts, score, shield, clock, boosts)
			end
		end
		table.clear(winningsOf)
		table.clear(boostsOf)
	end,
})
