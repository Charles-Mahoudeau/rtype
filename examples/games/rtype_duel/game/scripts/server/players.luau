-- The players: each of the first two clients gets a ship, removed when they leave.

-- The server's systems only: clients have none of the systems they are ordered against.
if not engine.isServer then
	return
end

print(`[server] waiting for {Duel.playerCount} players`)

-- Update: gives each of the first two clients a ship, kinematic: MoveShips moves it, the lasers hit it, nothing pushes
-- it.
engine.ecs.system("game.SpawnPlayers", {
	phase = "Update",
	side = "server",
	query = { read = { "game.Ship" } },
	events = { read = { "net.ClientConnected" } },
	run = function(ctx)
		local taken = 0
		local leftTaken = false
		for _, ship in ctx.query do
			taken += 1
			leftTaken = leftTaken or ship.player == 0
		end
		for joined in ctx.events.read("net.ClientConnected") do
			if taken >= Duel.playerCount then
				print(`[server] client {joined.client} joined, but the game is full`)
				continue
			end
			local player = if leftTaken then 1 else 0
			leftTaken = true
			taken += 1
			local x = if player == 0 then Duel.shipX else Duel.viewWidth - Duel.shipX
			local position = { x = x, y = Duel.viewHeight / 2 }
			ctx.commands:spawn({
				["game.Ship"] = { position = position, player = player },
				["net.Owner"] = { client = joined.client },
				["net.PlayerInput"] = {},
				["rtype.Transform"] = { position = position },
				["rtype.Collider"] = {
					size = { x = Duel.shipHalfSize.x * 2, y = Duel.shipHalfSize.y * 2 },
					layer = Duel.shipLayerOf(player),
					mask = bit32.bor(Duel.obstacleLayer, Duel.laserLayer),
				},
				["rtype.RigidBody"] = { type = Duel.kinematic },
			})
			print(`[server] client {joined.client} is player {player + 1}`)
		end
	end,
})

-- Update: removes the ship of a client who left.
engine.ecs.system("game.RemovePlayers", {
	phase = "Update",
	side = "server",
	query = { read = { "net.Owner" }, with = { "game.Ship" } },
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
