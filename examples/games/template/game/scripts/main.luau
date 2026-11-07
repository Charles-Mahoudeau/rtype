-- A component, and a system that counts: the smallest game that does something on the server.
-- Every file of `scripts/` is loaded, in sorted order, on the server and on every client.

engine.ecs.component("game.Counter", { fields = { value = "u32" } })

-- Runs once at start, on the server: spawns the entity the counter lives on.
engine.ecs.system("game.SpawnCounter", {
	phase = "Startup",
	query = {},
	run = function(ctx)
		ctx.commands:spawn({ ["game.Counter"] = { value = 0 } })
	end,
})

-- Says on the server's output when a player joins or leaves.
engine.ecs.system("game.LogPlayers", {
	phase = "Update",
	side = "server",
	query = {},
	events = { read = { "net.ClientConnected", "net.ClientDisconnected" } },
	run = function(ctx)
		for joined in ctx.events.read("net.ClientConnected") do
			print("client connected: " .. tostring(joined.client))
		end
		for left in ctx.events.read("net.ClientDisconnected") do
			print("client disconnected: " .. tostring(left.client))
		end
	end,
})

-- Runs every fixed step, on the server.
engine.ecs.system("game.Count", {
	phase = "FixedUpdate",
	query = { write = { "game.Counter" } },
	run = function(ctx)
		for entity, counter in ctx.query do
			counter.value += 1
		end
	end,
})
