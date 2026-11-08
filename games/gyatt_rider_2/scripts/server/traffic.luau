-- The traffic: vehicles and cyclists come from behind, below the view, in a free road, and slowly overtake the riders
-- up the view, each at the speed its capsule file gives. They leave through the top.

local Game = require("@scripts/shared/rules")

-- The capsule of each kind, how often it comes compared with the others, and the tiles of its road it may drive on,
-- from the road's middle: a vehicle in the middle, a cyclist on any of the three.
local middle = { 0 }
local anyTile = { -1, 0, 1 }
local kinds = {
	{ capsule = "actors/car.actor", weight = 6, tiles = middle },
	{ capsule = "actors/electric_car.actor", weight = 3, tiles = middle },
	{ capsule = "actors/van.actor", weight = 3, tiles = middle },
	{ capsule = "actors/truck.actor", weight = 2, tiles = middle },
	{ capsule = "actors/bus.actor", weight = 1, tiles = middle },
	{ capsule = "actors/limousine.actor", weight = 1, tiles = middle },
	{ capsule = "actors/cyclist.actor", weight = 4, tiles = anyTile },
}
local totalWeight = 0
for _, kind in kinds do
	totalWeight += kind.weight
end

local spawnDelayMin, spawnDelayMax = 1.2, 2.5 -- seconds between two arrivals
local longestTiles = 7 -- the limousine: a road's entrance is free when one fits there
local spawnY = Game.viewHeight + longestTiles / 2 * Game.tile -- out of view, the longest one included
local trafficMask = bit32.bor(Game.vehicleLayer, Game.cyclistLayer) -- what the traffic looks for in a road
local followGap = Game.tile / 2 -- how close a vehicle comes to the traffic ahead
local castThickness = 2 -- the height of the box a vehicle casts ahead of it
local castMargin = 2 -- narrower than the vehicle, so that the traffic of the next road, edge to edge, is not ahead

local countdown = 0 -- seconds before the next arrival

--- A kind drawn by weight.
local function drawKind()
	local roll = math.random() * totalWeight
	for _, kind in kinds do
		roll -= kind.weight
		if roll < 0 then
			return kind
		end
	end
	return kinds[#kinds]
end

-- FixedUpdate, once the physics stepped, so that it asks the scene of this tick: now and then, a vehicle of a random
-- kind enters a random road whose entrance is free, on one of the tiles its kind drives on.
engine.ecs.system("game.SpawnTraffic", {
	phase = "FixedUpdate",
	side = "server",
	query = {},
	calls = { "rtype.capsule.spawn", "rtype.physics.overlap" },
	after = { "rtype.Physics.Events" },
	run = function(ctx)
		countdown -= ctx.time.fixedDelta
		if countdown > 0 then
			return
		end
		countdown = spawnDelayMin + math.random() * (spawnDelayMax - spawnDelayMin)
		local roadX = Game.roadCenterX(math.random(0, Game.roadCount - 1))
		local entrance = ctx.call("rtype.physics.overlap", {
			shape = Game.box,
			size = Game.sizeOf({ x = Game.roadTiles, y = longestTiles }),
			at = { x = roadX, y = spawnY },
			mask = trafficMask,
		})
		if entrance.count > 0 then
			return -- the road is taken: the next arrival tries again
		end
		local kind = drawKind()
		local position = { x = roadX + kind.tiles[math.random(1, #kind.tiles)] * Game.tile, y = spawnY }
		engine.capsule.spawn(ctx, kind.capsule, {
			["rtype.Transform"] = { position = position },
			["game.Position"] = { value = position },
		})
	end,
})

-- FixedUpdate, after the riders moved and before the Transforms drive the bodies: each vehicle goes up at its speed,
-- but no closer than followGap to the traffic ahead. A thin box as wide as it, cast up from its front, finds it: a
-- cyclist on a side tile overlaps half a vehicle, which a single ray from the vehicle's middle would miss. It leaves
-- once it is out of the view's top.
engine.ecs.system("game.DriveTraffic", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Driving", "rtype.Collider" }, write = { "game.Position", "rtype.Transform" } },
	calls = { "rtype.physics.shapeCast" },
	after = { "rtype.Physics.SyncBodies", "game.MoveRiders" },
	before = { "rtype.Physics.SyncToPhysics" },
	run = function(ctx)
		for entity, driving, collider, current, transform in ctx.query do
			local step = driving.speed * ctx.time.fixedDelta
			local ahead = ctx.call("rtype.physics.shapeCast", {
				shape = Game.box,
				size = { x = collider.size.x - castMargin, y = castThickness },
				from = { x = current.value.x, y = current.value.y - (collider.size.y + castThickness) / 2 },
				direction = { x = 0, y = -1 },
				maxDistance = followGap + step,
				mask = trafficMask,
				ignore = entity,
			})
			if ahead.hit then
				step = math.max(0, ahead.distance - followGap)
			end
			if step == 0 then
				continue -- stuck behind slower traffic: not written, so not sent again
			end
			local position = { x = current.value.x, y = current.value.y - step }
			if position.y + collider.size.y / 2 < 0 then
				ctx.commands:despawn(entity)
				continue
			end
			transform.position = position
			current.value = position
		end
	end,
})
