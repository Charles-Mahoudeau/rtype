-- The players: a client joins a team with game.Join and gets a round body on the pitch, removed when it spectates or
-- leaves. Players accelerate toward their input, the damping caps their speed, and a kick pushes the ball away.

local Game = require("@scripts/shared/rules")
local State = require("@scripts/server/state")

--- A name a player may have: printable, at most Game.nameLength bytes, or a default one.
local function cleanName(name: string, client: number): string
	local clean = string.gsub(name, "%c", "")
	clean = string.match(clean, "^%s*(.-)%s*$") or ""
	if clean == "" then
		return `Player {client}`
	end
	return string.sub(clean, 1, Game.nameLength)
end

--- How many players each team has, from the team of each player (by its entity's bits).
local function countTeams(players: { [number]: number }): { [number]: number }
	local counts = { [Game.red] = 0, [Game.blue] = 0 }
	for _, team in players do
		if counts[team] ~= nil then
			counts[team] += 1
		end
	end
	return counts
end

-- Update: the clients' choices of team. A new player starts on its half; one changing teams moves to the other half.
engine.ecs.system("game.JoinTeams", {
	phase = "Update",
	side = "server",
	query = { read = { "net.Owner" }, write = { "game.Player" } },
	events = { read = { "net.PacketReceived" } },
	run = function(ctx)
		local packets = {}
		for _, packet in engine.net.received(ctx) do
			if packet.name == "game.Join" then
				table.insert(packets, packet)
			end
		end
		if #packets == 0 then
			return
		end

		local byClient = {}
		local teamOf = {}
		for entity, owner, player in ctx.query do
			byClient[owner.client] = entity
			teamOf[entity.bits] = player.team
		end

		for _, packet in packets do
			local client, team = packet.sender, packet.values.team
			local name = cleanName(packet.values.name, client)
			local existing = byClient[client]
			local counts = countTeams(teamOf)
			if team ~= Game.red and team ~= Game.blue then
				-- Spectating, or a team that does not exist: off the pitch.
				if existing ~= nil then
					ctx.commands:despawn(existing)
					teamOf[existing.bits] = nil
					byClient[client] = nil
					print(`[server] {name} spectates`)
				end
				continue
			end
			if existing ~= nil and teamOf[existing.bits] == team then
				ctx.commands:patch(existing, "game.Player", { name = name })
				continue
			end
			if counts[team] >= Game.teamSize then
				print(`[server] {name} cannot join {Game.teamNames[team]}: it is full`)
				continue
			end
			local spot = Game.spotOf(team, counts[team] + 1, false)
			if existing ~= nil then
				ctx.commands:patch(existing, "game.Player", { team = team, name = name })
				ctx.commands:patch(existing, "rtype.Transform", { position = spot })
				ctx.commands:patch(existing, "rtype.Velocity", { linear = { x = 0, y = 0 } })
				teamOf[existing.bits] = team
			else
				local entity = ctx.commands:spawn({
					["game.Player"] = { team = team, name = name, position = spot },
					["game.Kicker"] = {},
					["game.Buffs"] = {},
					["rtype.Transform"] = { position = spot },
					["rtype.Collider"] = {
						shape = Game.circleShape,
						radius = Game.playerRadius,
						restitution = Game.bounce,
						friction = 0,
						layer = Game.playerLayer,
						mask = bit32.bor(Game.playerLayer, Game.ballLayer, Game.playerWallLayer, Game.postLayer),
					},
					["rtype.RigidBody"] = {
						type = Game.dynamicBody,
						mass = Game.playerMass,
						linearDamping = Game.playerDamping,
						gravityScale = 0,
						fixedRotation = true,
					},
					["rtype.Velocity"] = {},
					["net.Owner"] = { client = client },
					["net.PlayerInput"] = {},
				})
				byClient[client] = entity
				teamOf[entity.bits] = team
			end
			print(`[server] {name} plays for {Game.teamNames[team]}`)
		end
	end,
})

-- Update: removes the player of a client who left.
engine.ecs.system("game.RemovePlayers", {
	phase = "Update",
	side = "server",
	query = { read = { "net.Owner" }, with = { "game.Player" } },
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

-- Whether each player is big (its collider and mass changed), by entity bits: a collider is only patched on a change,
-- since a changed collider rebuilds the body.
local isBig: { [number]: boolean } = {}

-- FixedUpdate, after the inputs of the tick and before the velocities drive the bodies: the power-ups' clocks, then
-- moves, dashes and kicks.
engine.ecs.system("game.MovePlayers", {
	phase = "FixedUpdate",
	side = "server",
	query = {
		read = { "rtype.Transform" },
		write = { "game.Player", "game.Kicker", "game.Buffs", "rtype.Velocity" },
		with = { "net.PlayerInput" },
	},
	calls = { "rtype.replication.readInput", "rtype.physics.applyImpulse", "rtype.net.sendToClients" },
	after = { "net.applyInputs", "rtype.Physics.SyncBodies" },
	before = { "rtype.Physics.SyncToPhysics" },
	run = function(ctx)
		local dt = ctx.time.fixedDelta
		local frozen = State.frozen()
		local ball = State.ball()
		local ballAt = State.ballPosition()

		for entity, transform, player, kicker, buffs, velocity in ctx.query do
			-- The power-ups run out; game.Player.buffs tells the clients which are left.
			buffs.speed = math.max(0, buffs.speed - dt)
			buffs.big = math.max(0, buffs.big - dt)
			buffs.direct = math.max(0, buffs.direct - dt)
			buffs.strongKick = math.max(0, buffs.strongKick - dt)
			local mask = 0
			if buffs.speed > 0 then
				mask += bit32.lshift(1, Game.speed - 1)
			end
			if buffs.big > 0 then
				mask += bit32.lshift(1, Game.big - 1)
			end
			if buffs.direct > 0 then
				mask += bit32.lshift(1, Game.direct - 1)
			end
			if buffs.strongKick > 0 then
				mask += bit32.lshift(1, Game.strongKick - 1)
			end
			if player.buffs ~= mask then
				player.buffs = mask
			end
			local big = buffs.big > 0
			if big ~= (isBig[entity.bits] or false) then
				isBig[entity.bits] = big
				local factor = if big then Game.bigFactor else 1
				ctx.commands:patch(entity, "rtype.Collider", { radius = Game.playerRadius * factor })
				ctx.commands:patch(entity, "rtype.RigidBody", { mass = Game.playerMass * (if big then Game.bigMassFactor else 1) })
			end
			local reach = Game.playerRadius * (if big then Game.bigFactor else 1) + Game.ballRadius + Game.kickReach

			local kick = ctx.call("rtype.replication.readInput", { entity = entity, action = "kick" })
			player.kicking = kick.held
			if kick.pressed then
				kicker.armed = true
			elseif not kick.held then
				kicker.armed = false
			end
			kicker.cooldown = math.max(0, kicker.cooldown - dt)
			if player.dash > 0 then
				player.dash = math.max(0, player.dash - dt)
			end

			if frozen then
				velocity.linear = { x = 0, y = 0 }
				continue
			end

			-- Toward the input, a diagonal as fast as a straight line; up is +1 in the input and Y goes down.
			local move = ctx.call("rtype.replication.readInput", { entity = entity, action = "move" }).vector
			local length = Game.length(move)
			if length > 1 then
				move = { x = move.x / length, y = move.y / length }
				length = 1
			end
			local acceleration = if kick.held then Game.kickingAcceleration else Game.playerAcceleration
			if buffs.speed > 0 then
				acceleration *= Game.speedFactor
			end
			local linear = velocity.linear
			if buffs.direct > 0 then
				-- No inertia: the velocity is the input times the top speed, so a player stops and turns at once.
				local top = acceleration / Game.playerDamping
				velocity.linear = { x = move.x * top, y = -move.y * top }
			else
				velocity.linear = {
					x = linear.x + move.x * acceleration * dt,
					y = linear.y - move.y * acceleration * dt,
				}
			end

			-- The dash: once per press, along the input (or along the velocity when no key is held).
			local dash = ctx.call("rtype.replication.readInput", { entity = entity, action = "dash" })
			if dash.pressed and player.dash <= 0 then
				local direction = { x = move.x, y = -move.y }
				if length < 0.1 then
					local speed = Game.length(velocity.linear)
					direction = if speed > 20 then { x = velocity.linear.x / speed, y = velocity.linear.y / speed } else { x = 0, y = 0 }
				else
					direction = { x = direction.x / length, y = direction.y / length }
				end
				if direction.x ~= 0 or direction.y ~= 0 then
					local now = velocity.linear
					velocity.linear = { x = now.x + direction.x * Game.dashSpeed, y = now.y + direction.y * Game.dashSpeed }
					player.dash = Game.dashCooldown
					local at = transform.position
					engine.net.sendToClients(ctx, "game.Dashed", { x = at.x, y = at.y })
				end
			end

			-- The kick: once per press, as soon as the ball is in reach.
			if kicker.armed and kicker.cooldown <= 0 and ball ~= nil then
				local position = transform.position
				local away = { x = ballAt.x - position.x, y = ballAt.y - position.y }
				local distance = Game.length(away)
				if distance > 0 and distance <= reach then
					local direction = { x = away.x / distance, y = away.y / distance }
					local kickSpeed = Game.kickSpeed * (if buffs.strongKick > 0 then Game.strongKickFactor else 1)
					local impulse = kickSpeed * Game.ballMass
					ctx.call("rtype.physics.applyImpulse", {
						entity = ball,
						value = { x = direction.x * impulse, y = direction.y * impulse },
					})
					kicker.armed = false
					kicker.cooldown = Game.kickCooldown
					State.touched(player.name, player.team)
					local after = State.ballVelocity()
					local power = Game.length({
						x = after.x + direction.x * kickSpeed,
						y = after.y + direction.y * kickSpeed,
					})
					engine.net.sendToClients(ctx, "game.Kicked", { x = ballAt.x, y = ballAt.y, power = power })
				end
			end
		end
	end,
})

-- FixedUpdate, once the physics moved the bodies: copies them to game.Player for the clients, and counts the teams.
engine.ecs.system("game.MirrorPlayers", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "rtype.Transform", "rtype.Velocity" }, write = { "game.Player" } },
	after = { "rtype.Physics.SyncFromPhysics" },
	run = function(ctx)
		local red, blue = 0, 0
		for _, transform, velocity, player in ctx.query do
			player.position = transform.position
			player.velocity = velocity.linear
			if player.team == Game.red then
				red += 1
			elseif player.team == Game.blue then
				blue += 1
			end
		end
		State.setTeamCounts(red, blue)
	end,
})

-- FixedUpdate, before the bodies move: puts every player on its kickoff spot when the match asks for it.
local playersReset = 0
engine.ecs.system("game.ResetPlayers", {
	phase = "FixedUpdate",
	side = "server",
	query = { read = { "game.Player" }, write = { "rtype.Transform", "rtype.Velocity" } },
	after = { "rtype.Physics.SyncBodies", "game.MovePlayers" },
	before = { "rtype.Physics.SyncToPhysics" },
	run = function(ctx)
		local match = State.match()
		if match.resets == playersReset then
			return
		end
		playersReset = match.resets
		local placed = { [Game.red] = 0, [Game.blue] = 0 }
		for _, player, transform, velocity in ctx.query do
			if placed[player.team] ~= nil then
				placed[player.team] += 1
				transform.position = Game.spotOf(player.team, placed[player.team], player.team == match.kickoffTeam)
				velocity.linear = { x = 0, y = 0 }
			end
		end
	end,
})
