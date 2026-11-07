-- The rules both sides share: the roads, the replicated components and the input actions. Files load in sorted path
-- order on the server and on the clients, so this one, first, gives the others the `Game` table; the manifest checks
-- that both sides declared the same components and actions.

Game = {
	tile = 64, -- pixels per tile: every size of the game is counted in tiles
	roadTiles = 2, -- a road is as wide as a vehicle
	roadCount = 8,
	sidebarTiles = 2, -- the strip on the left, under the timer and the gauge
	viewWidth = 1152,
	viewHeight = 640,
	maxPlayers = 4, -- the next players watch
	riderTiles = { x = 1, y = 2 },
	riderSpeed = 400, -- world units per second, in every direction
	startRoads = { 1, 3, 4, 6 }, -- the road each slot starts on, counted from 0 on the left
	startY = 512, -- where the riders start, near the bottom

	-- The collision categories: each Collider belongs to some (layer) and collides with some (mask).
	riderLayer = 1,
	vehicleLayer = 2,
	bulletLayer = 4,
	cyclistLayer = 8,
}

-- The roads' edges, in world units: the riders stay inside.
Game.roadsLeft = Game.sidebarTiles * Game.tile
Game.roadsRight = Game.viewWidth

-- The enums of the engine's components, as their reflected numbers.
Game.box = 0 -- rtype.Collider shape
Game.kinematic = 1 -- rtype.RigidBody type

--- The size of something counted in tiles, in world units.
function Game.sizeOf(tiles: { x: number, y: number }): { x: number, y: number }
	return { x = tiles.x * Game.tile, y = tiles.y * Game.tile }
end

--- The X of a road's middle, the road counted from 0 on the left.
function Game.roadCenterX(road: number): number
	return Game.roadsLeft + (road + 0.5) * Game.roadTiles * Game.tile
end

--- Where the rider of a slot starts.
function Game.startOf(slot: number): { x: number, y: number }
	return { x = Game.roadCenterX(Game.startRoads[slot + 1]), y = Game.startY }
end

-- A player's rider, on every change. Its slot (0 to 3) gives its start and its color.
engine.ecs.component("game.Rider", {
	fields = { position = "vec2", slot = "u32" },
	replication = "everyChange",
})

-- The actions both sides agree on: the clients send them, the server reads them. The keys are physical positions
-- named after the US layout: W A S D is Z Q S D on a French keyboard.
engine.input.action("move", "vector2", { bind = { { up = "W", down = "S", left = "A", right = "D" } } })
