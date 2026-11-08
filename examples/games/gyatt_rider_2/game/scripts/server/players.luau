-- The players: each of the first four clients gets a rider, removed when they leave. The next ones watch.

local Game = require("@scripts/shared/rules")

print(`[server] up to {Game.maxPlayers} players`)

-- Update: gives each new client a rider in the first free slot, kinematic: the riders' moves place it, nothing pushes
-- it.
engine.ecs.system("game.SpawnRiders", {
	phase = "Update",
	side = "server",
	query = { read = { "game.Rider" } },
	events = { read = { "net.ClientConnected" } },
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
			ctx.commands:spawn({
				["game.Rider"] = { position = position, slot = slot },
				["net.Owner"] = { client = joined.client },
				["net.PlayerInput"] = {},
				["rtype.Transform"] = { position = position },
				["rtype.Collider"] = {
					shape = Game.box,
					size = Game.riderSize,
					layer = Game.riderLayer,
					mask = bit32.bor(Game.vehicleLayer, Game.bulletLayer),
				},
				["rtype.RigidBody"] = { type = Game.kinematic },
			})
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
