-- The rules both sides share: the arena, the replicated components and the input actions. Files load in sorted path
-- order on the server and on the clients, so this one, first, gives the others the `Duel` table; the manifest checks
-- that both sides declared the same components and actions.

Duel = {
	viewWidth = 800,
	viewHeight = 600,
	middleX = 400, -- each ship stays on its side of it
	shipSpeed = 300, -- world units per second
	laserSpeed = 600,
	shipHalfSize = { x = 24, y = 24 },
	laserHalfSize = { x = 30, y = 6 },
	playerCount = 2,
	misfireOdds = 10, -- one shot in ten misfires
	misfireLifetime = 0.25, -- seconds a Misfire lives on the server, long enough for a snapshot to carry it

	-- The collision categories: each Collider belongs to some (layer) and collides with some (mask).
	obstacleLayer = 1,
	laserLayer = 2,
	leftShipLayer = 4,
	rightShipLayer = 8,

	-- The capsules (assets/): the arena and what the server spawns in it.
	arena = "scenes/arena.scene",
	ships = { [0] = "actors/ship_left.actor", [1] = "actors/ship_right.actor" },
	laser = "actors/laser.actor",

	-- Whether players join through the network. Run alone (`aylo-runtime --editor`), the server and the client are one
	-- process with no network: there are no players, and the scripts that need them stay out.
	networked = not (engine.isServer and engine.isClient),
}

-- The enums of the engine's components, as their reflected numbers.
Duel.box = 0 -- rtype.Collider shape
Duel.circle = 1
Duel.kinematic = 1 -- rtype.RigidBody type
Duel.additive = 1 -- rtype.SpriteRenderer blend

--- +1 for the left player (0), who fires right; -1 for the right one (1).
function Duel.directionOf(player: number): number
	return if player == 0 then 1 else -1
end

--- The collision layer of a player's ship.
function Duel.shipLayerOf(player: number): number
	return if player == 0 then Duel.leftShipLayer else Duel.rightShipLayer
end

-- A player's ship, on every change.
engine.ecs.component("game.Ship", {
	fields = { position = "vec2", player = "u32" }, -- player 0 is on the left, firing right; 1 on the right
	replication = "everyChange",
})
-- A laser in flight: it flies away from its shooter and cannot hit them.
engine.ecs.component("game.Laser", {
	fields = { position = "vec2", shooter = "u32" },
	replication = "everyChange",
})
-- A wall in the middle of the arena: the clients place and draw it.
engine.ecs.component("game.Obstacle", { fields = { position = "vec2" }, replication = "everyChange" })
-- Spawned by the server when a laser hits: the game is over.
engine.ecs.component("game.Winner", { fields = { player = "u32" }, replication = "everyChange" })
-- Spawned by the server instead of a laser when a shot misfires, so that the clients play the misfire sound.
engine.ecs.component("game.Misfire", {
	fields = { position = "vec2", shooter = "u32" },
	replication = "everyChange",
})
-- Server only: how long a Misfire still lives.
engine.ecs.component("game.MisfireTimer", { fields = { remaining = "f32" }, presence = "serverOnly" })

-- The actions both sides agree on: the clients send them, the server reads them.
engine.input.action("move", "vector2", { -- up and down (only Y is used)
	bind = {
		{ up = "W", down = "S", left = "A", right = "D" },
		{ up = "Up", down = "Down", left = "Left", right = "Right" },
	},
})
engine.input.action("shoot", "button", { bind = { "MouseLeft" } })

-- Startup, on every side: the arena. The server spawns its obstacles and replicates them to the clients.
engine.ecs.system("game.LoadArena", {
	phase = "Startup",
	side = "both",
	query = {},
	calls = { "rtype.capsule.load" },
	run = function(ctx)
		engine.capsule.load(ctx, Duel.arena)
	end,
})
