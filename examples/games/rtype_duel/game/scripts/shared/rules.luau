-- The replicated components and the input actions both sides declare: the manifest checks that the server and the
-- clients declared the same ones. Required by shared/main.luau, which every process runs first.

local Duel = require("@scripts/shared/arena")

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
