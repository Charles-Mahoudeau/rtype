-- The players: each of the first four clients gets a rider, removed when they leave. The next ones watch.

local Game = require("@scripts/shared/rules")

print(`[server] up to {Game.maxPlayers} players`)

-- Update: gives each new client a rider (assets/actors/rider.actor) in the first free slot.
engine.ecs.system("game.SpawnRiders", {
	phase = "Update",
	side = "server",
	query = { read = { "game.Rider" } },
	events = { read = { "net.ClientConnected" } },
	calls = { "rtype.capsule.spawn" },
	run = function(ctx)
		local taken = {}
		for _, rider in ctx.query do
			taken[rider.slot] = true
		end
		for joined in ctx.events.read("net.ClientConnected") do
			local slot = nil
			for candidate = 0, Game.maxPlayers - 1 do
				if not taken[candidate] then
					slot = candidate
					break
				end
			end
			if slot == nil then
				print(`[server] client {joined.client} joined, but the game is full: they watch`)
				continue
			end
			taken[slot] = true
			local position = Game.startOf(slot)
			-- The rider's actor holds its body, its gun and its draw layer; the slot gives its start. Every rider is the
			-- white cyclist's image, which each client tints: blue for its own, red for the others.
			local rider = engine.capsule.spawn(ctx, Game.riderActor, {
				["game.Rider"] = { slot = slot },
				["game.Position"] = { value = position },
				["game.Look"] = { texture = "cyclists/cyclist_sheet.png" },
				["rtype.Transform"] = { position = position },
			})
			ctx.commands:add(rider, "net.Owner", { client = joined.client })
			ctx.commands:add(rider, "net.PlayerInput", {})
			print(`[server] client {joined.client} is player {slot + 1}`)
		end
	end,
})

-- Update: removes the rider of a client who left, which frees its slot.
engine.ecs.system("game.RemoveRiders", {
	phase = "Update",
	side = "server",
	query = { read = { "net.Owner" }, with = { "game.Rider" } },
	events = { read = { "net.ClientDisconnected" } },
	run = function(ctx)
		local left = {}
		for client in ctx.events.read("net.ClientDisconnected") do
			left[client.client] = true
		end
		if next(left) == nil then
			return
		end
		for entity, owner in ctx.query do
			if left[owner.client] then
				ctx.commands:despawn(entity)
				print(`[server] client {owner.client} left`)
			end
		end
	end,
})
